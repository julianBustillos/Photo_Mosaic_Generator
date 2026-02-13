#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
#include "ImageUtils.h"
#include "ColorUtils.h"
#include <numbers>


ColorEnhancer::ColorEnhancer(const ProbaUtils::Histogram<3>& sourceHistogram, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm, const ProbaUtils::GMMSamplerDatas<3>& datas) :
    _sourceHistogram(sourceHistogram), _sourceGmm(sourceGmm), _targetGmm(targetGmm), _datas(datas), _blendingScale(1.)
{
    const int nbValues = _sourceHistogram._values.size();
    _histCompProbas.resize(_sourceGmm.size() * nbValues);
    std::vector<double> histProbaNorms(nbValues);
    ProbaUtils::evalGaussianPDF(_histCompProbas, histProbaNorms, _sourceHistogram._values, sourceGmm, true);

    double distance = ProbaUtils::computeGmmW2<3>(_wstar, _sourceGmm, _targetGmm);

    double sourceCoverage = computeGMMSamplingCoverage(sourceGmm, CoverageGridDivisions, CoverageConfidence);
    double targetCoverage = computeGMMSamplingCoverage(targetGmm, CoverageGridDivisions, CoverageConfidence);
    
    double minCoverage = CoverageMinRatio * sourceCoverage;
    if (targetCoverage < minCoverage)
        _blendingScale = goldenSectionSearch(minCoverage);
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::apply(cv::Mat& enhancedImage, double blending)
{
    std::vector<MathUtils::VectorNd<3>> colorMap(_sourceHistogram._values.size(), MathUtils::VectorNd<3>::Zero());
    computeColorMap(colorMap, blending * _blendingScale);

    //Compute color enhanced color image
    const int nbPixels = enhancedImage.rows * enhancedImage.cols;
    std::vector<double> img(nbPixels * 3);
    std::vector<double> enhancedDiff(nbPixels * 3);
    for (int p = 0, k = 0; p < nbPixels; p++)
    {
        int mapId = _sourceHistogram._mapId[p];
        const MathUtils::VectorNd<3>& pixel = _sourceHistogram._values[mapId];
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

double ColorEnhancer::computeGMMSamplingCoverage(const ProbaUtils::GMMNDComponents<3>& gmm, int nbDivisions, double confidence)
{
    ProbaUtils::GMMSamples<3> samples;
    ProbaUtils::computeGmmSamples(samples, gmm, _datas);

    const int nbSamples = samples.size();
    int nbValidSamples = 0;
    const int nbBins = nbDivisions * nbDivisions * nbDivisions;
    std::vector<int> histogram(nbBins, 0);
    double step = 255. / nbDivisions;

    for (int s = 0; s < nbSamples; s++)
    {
        int histIdx = 0;
        int factor = 1;
        for (int i = 0; i < 3; i++)
        {
            int gridIdx = (int)std::floor(samples[s](i, 0) / step);
            if (gridIdx < 0 || nbDivisions <= gridIdx)
            {
                histIdx = -1;
                break;
            }

            histIdx += factor * gridIdx;
            factor *= nbDivisions;
        }

        if (histIdx >= 0)
        {
            histogram[histIdx] += 1;
            nbValidSamples++;
        }
    }
    std::sort(histogram.begin(), histogram.end(), std::greater<int>());

    double coverage = 0;
    int cumulatedNbSamples = 0;
    double confidenceLimit = confidence * nbValidSamples;
    for (int b = 0; b < nbBins; b++)
    {
        cumulatedNbSamples += histogram[b];
        if (cumulatedNbSamples >= confidenceLimit)
        {
            double binRatio = (histogram[b] - cumulatedNbSamples + confidenceLimit) / histogram[b];
            coverage = (b + binRatio) / nbBins;
            break;
        }
    }

    return coverage;
}

double ColorEnhancer::computeCoverageDist(double t, double coverage)
{
    ProbaUtils::GMMNDComponents<3> gmmt;
    ProbaUtils::computeGmmInterpolation(gmmt, t, _sourceGmm, _targetGmm, _wstar);
    double coveraget = computeGMMSamplingCoverage(gmmt, CoverageGridDivisions, CoverageConfidence);

    return abs(coverage - coveraget);
}

void ColorEnhancer::computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double t) const
{
    const int nbComponents = _sourceGmm.size();
    const int nbValues = _sourceHistogram._values.size();
    const int wstarSize = _wstar.size();

    ProbaUtils::GMMNDComponents<3> gmmt;
    ProbaUtils::computeGmmInterpolation<3>(gmmt, t, _sourceGmm, _targetGmm, _wstar);

    std::vector<MathUtils::MatrixNd<3>> transferMap(wstarSize);
    for (int w = 0; w < wstarSize; w++)
    {
        double k = _wstar[w]._k;
        const MathUtils::MatrixNd<3>& sigma0 = _sourceGmm[k]._covariance;
        const MathUtils::MatrixNd<3>& sigmaT = gmmt[w]._covariance;
        transferMap[w] = (MathUtils::inv<3>(sigma0) * MathUtils::sqrt<3>(sigma0 * sigmaT)).transpose();
    }

    for (int h = 0; h < nbValues; h++)
    {
        for (int w = 0; w < wstarSize; w++)
        {
            double k = _wstar[w]._k;
            colorMap[h] += gmmt[w]._weight / _sourceGmm[k]._weight * _histCompProbas[h * nbComponents + k] * (gmmt[w]._mean + transferMap[w] * (_sourceHistogram._values[h] - _sourceGmm[k]._mean));
        }

        colorMap[h] = colorMap[h].cwiseMax(0).cwiseMin(255); //clipping
    }
}

