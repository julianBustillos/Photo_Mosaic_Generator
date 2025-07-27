#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
#include "ImageUtils.h"
#include "ColorUtils.h"
#include <numbers>


std::vector<ProbaUtils::Bin<3>> ColorEnhancer::ColorSpace;

void ColorEnhancer::initializeColorSpace(double valueMin, double valueMax, int nbElements, int nbDivisions)
{
    ColorSpace.clear();
    ColorSpace.reserve(nbDivisions * nbDivisions * nbDivisions);

    double divSize = (double)nbElements / (double)nbDivisions;
    double firstPos = (divSize - 1.) * 0.5;
    double valueRange = valueMax - valueMin;
    double valueScale = valueRange / ((double)nbElements - 1.);

    ProbaUtils::Bin<3> currBin;
    currBin._count = 1;
    for (int i = 0; i < nbDivisions; i++)
    {
        currBin._value[0] = valueMin + (firstPos + i * divSize) * valueScale;
        for (int j = 0; j < nbDivisions; j++)
        {
            currBin._value[1] = valueMin + (firstPos + j * divSize) * valueScale;
            for (int k = 0; k < nbDivisions; k++)
            {
                currBin._value[2] = valueMin + (firstPos + k * divSize) * valueScale;
                ColorSpace.emplace_back(currBin);
            }
        }
    }
}

ColorEnhancer::ColorEnhancer(const ProbaUtils::SampleData<3>& sourceSample, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm) :
    _sourceSample(sourceSample), _sourceGmm(sourceGmm), _targetGmm(targetGmm)
{
    const int histogramSize = _sourceSample._histogram.size();
    _histCompProbas.resize(_sourceGmm.size() * histogramSize);
    std::vector<double> histProbaNorms(histogramSize);
    ProbaUtils::evalGaussianPDF(_histCompProbas, histProbaNorms, sourceSample._histogram, sourceGmm, true);

    double distance = ProbaUtils::computeGmmW2<3>(_wstar, _sourceGmm, _targetGmm);

    _sourceCoverage = ProbaUtils::computeHistCoverage(sourceGmm, ColorSpace, CoverageMinDensity);
    _targetCoverage = ProbaUtils::computeHistCoverage(targetGmm, ColorSpace, CoverageMinDensity);
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::apply(cv::Mat& enhancedImage, double blending) const
{
    double t = 0.;
    if (blending > 0)
    {
        double blendingCoverage = (1 - blending) * _sourceCoverage + blending * CoverageMinRatio * _sourceCoverage;
        if (blendingCoverage < _targetCoverage)
            t = 1.;
        else
            t = goldenSectionSearch(blendingCoverage);
    }

    std::vector<MathUtils::VectorNd<3>> colorMap(_sourceSample._histogram.size(), MathUtils::VectorNd<3>::Zero());
    computeColorMap(colorMap, t);

    //Compute color enhanced color image
    const int nbPixels = enhancedImage.rows * enhancedImage.cols;
    std::vector<double> img(nbPixels * 3);
    std::vector<double> enhancedDiff(nbPixels * 3);
    for (int p = 0, k = 0; p < nbPixels; p++)
    {
        int mapId = _sourceSample._mapId[p];
        const MathUtils::VectorNd<3>& pixel = _sourceSample._histogram[mapId]._value;
        for (int c = 0; c < 3; c++, k++)
        {
            img[k] = pixel(c);
            enhancedDiff[k] = colorMap[mapId](c) - img[k];
        }
    }

    //Compute correction to avoid artifacts on enhanced image
    std::vector<double> filtered(nbPixels * 3);
    ImageUtils::guidedFiltering(filtered, enhancedDiff, img, enhancedImage.size(), FilterRadius, FilterEpsilon);

    for (int k = 0; k < nbPixels * 3; k++)
        enhancedImage.data[k] = ColorUtils::clip<double>(img[k] + filtered[k], 0, 255);
}

double ColorEnhancer::goldenSectionSearch(double coverage) const
{
    static const double invPhi = 1. / std::numbers::phi;

    double tmin = 0, tmax = 1, t0 = -1, t1 = -1, ft0 = -1, ft1 = -1;
    ProbaUtils::GMMNDComponents<3> gmm0, gmm1;
    bool compute0 = true, compute1 = true;

    double h = tmax - tmin;
    while (h > GSSTolerance)
    {
        h = h * invPhi;

        if (compute0)
        {
            t0 = tmax - h;
            ProbaUtils::computeGmmInterp(gmm0, t0, _sourceGmm, _targetGmm, _wstar);
            ft0 = abs(ProbaUtils::computeHistCoverage(gmm0, ColorSpace, CoverageMinDensity) - coverage);
        }

        if (compute1)
        {
            t1 = tmin + h;
            ProbaUtils::computeGmmInterp(gmm1, t1, _sourceGmm, _targetGmm, _wstar);
            ft1 = abs(ProbaUtils::computeHistCoverage(gmm1, ColorSpace, CoverageMinDensity) - coverage);
        }

        if (ft0 < ft1)
        {
            tmax = t1;
            t1 = t0;
            ft1 = ft0;
            compute0 = true;
            compute1 = false;
        }
        else
        {
            tmin = t0;
            t0 = t1;
            ft0 = ft1;
            compute0 = false;
            compute1 = true;
        }
    }

    return (tmin + tmax) * 0.5;
}

void ColorEnhancer::computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double t) const
{
    const int nbComponents = _sourceGmm.size();
    const int histogramSize = _sourceSample._histogram.size();
    const int wstarSize = _wstar.size();

    ProbaUtils::GMMNDComponents<3> gmmt;
    ProbaUtils::computeGmmInterp<3>(gmmt, t, _sourceGmm, _targetGmm, _wstar);

    std::vector<MathUtils::MatrixNd<3>> transferMap(wstarSize);
    for (int w = 0; w < wstarSize; w++)
    {
        double k = _wstar[w]._k;
        const MathUtils::MatrixNd<3>& sigma0 = _sourceGmm[k]._covariance;
        const MathUtils::MatrixNd<3>& sigmaT = gmmt[k]._covariance;
        transferMap[w] = (MathUtils::inv<3>(sigma0) * MathUtils::sqrt<3>(sigma0 * sigmaT)).transpose();
    }

    for (int h = 0; h < histogramSize; h++)
    {
        for (int w = 0; w < wstarSize; w++)
        {
            double k = _wstar[w]._k;
            colorMap[h] += gmmt[w]._weight / _sourceGmm[k]._weight * _histCompProbas[h * nbComponents + k] * (gmmt[w]._mean + transferMap[w] * (_sourceSample._histogram[h]._value - _sourceGmm[k]._mean));
        }

        colorMap[h] = colorMap[h].cwiseMax(0).cwiseMin(255); //clipping
    }
}

