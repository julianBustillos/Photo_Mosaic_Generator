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
    _sourceSample(sourceSample), _sourceGmm(sourceGmm), _targetGmm(targetGmm), _blendingScale(1.)
{
    const int histogramSize = _sourceSample._histogram.size();
    _histCompProbas.resize(_sourceGmm.size() * histogramSize);
    std::vector<double> histProbaNorms(histogramSize);
    ProbaUtils::evalGaussianPDF(_histCompProbas, histProbaNorms, sourceSample._histogram, sourceGmm, true);

    double distance = ProbaUtils::computeGmmW2<3>(_wstar, _sourceGmm, _targetGmm);

    double sourceCoverage = ProbaUtils::computeHistCoverage(sourceGmm, ColorSpace, CoverageMinDensity);
    double targetCoverage = ProbaUtils::computeHistCoverage(targetGmm, ColorSpace, CoverageMinDensity);
    
    double minCoverage = CoverageMinRatio * sourceCoverage;
    if (targetCoverage < minCoverage)
        _blendingScale = goldenSectionSearch(minCoverage);
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::apply(cv::Mat& enhancedImage, double blending)
{
    std::vector<MathUtils::VectorNd<3>> colorMap(_sourceSample._histogram.size(), MathUtils::VectorNd<3>::Zero());
    computeColorMap(colorMap, blending * _blendingScale);

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

double ColorEnhancer::goldenSectionSearch(double coverage)
{
    static const double invPhi = 1. / std::numbers::phi;

    double tmin = 0, tmax = 1, t0 = -1, t1 = -1, dist0 = -1, dist1 = -1;
    bool compute0 = true, compute1 = true;

    double h = tmax - tmin;
    while (h > GSSTolerance)
    {
        h = h * invPhi;

        if (compute0)
        {
            t0 = tmax - h;
            dist0 = computeCoverageDist(t0, coverage);
        }

        if (compute1)
        {
            t1 = tmin + h;
            dist1 = computeCoverageDist(t1, coverage);
        }

        if (dist0 < dist1)
        {
            tmax = t1;
            t1 = t0;
            dist1 = dist0;
            compute0 = true;
            compute1 = false;
        }
        else
        {
            tmin = t0;
            t0 = t1;
            dist0 = dist1;
            compute0 = false;
            compute1 = true;
        }
    }

    return (tmin + tmax) * 0.5;
}

double ColorEnhancer::computeCoverageDist(double t, double coverage)
{
    double coveraget = 0;

    auto it = _coverageCacheMap.find(t);
    if (it == _coverageCacheMap.end())
    {
        ProbaUtils::GMMNDComponents<3> gmmt;
        ProbaUtils::computeGmmInterp(gmmt, t, _sourceGmm, _targetGmm, _wstar);
        coveraget = ProbaUtils::computeHistCoverage(gmmt, ColorSpace, CoverageMinDensity);
        _coverageCacheMap.insert(std::make_pair(t, coveraget));
    }
    else
        coveraget = it->second;

    return abs(coverage - coveraget);
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
        const MathUtils::MatrixNd<3>& sigmaT = gmmt[w]._covariance;
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

