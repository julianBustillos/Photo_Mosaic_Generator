#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
#include "ImageUtils.h"
#include "ColorUtils.h"
#include <numbers>


ColorEnhancer::ColorEnhancer(const ProbaUtils::SampleData<3>& sourceSample, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm) :
    _sourceSample(sourceSample), _sourceGmm(sourceGmm), _targetGmm(targetGmm)
{
    _histCompDensities.resize(_sourceGmm.size() * _sourceSample._histogram.size());
    ProbaUtils::evalGaussianPDF(_histCompDensities, sourceSample, sourceGmm);

    _histDensity.resize(_sourceSample._histogram.size(), 0.);
    for (int h = 0, d = 0; h < _histDensity.size(); h++)
    {
        for (int c = 0; c < _sourceGmm.size(); c++, d++)
            _histDensity[h] += _sourceGmm[c]._weight * _histCompDensities[d];
    }

    double distance = ProbaUtils::computeGmmW2<3>(_wstar, _sourceGmm, _targetGmm);
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::apply(cv::Mat& enhancedImage, double blending) const
{
    std::vector<MathUtils::VectorNd<3>> colorMap(_sourceSample._histogram.size(), MathUtils::VectorNd<3>::Zero());
    computeColorMap(colorMap, blending);

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

void ColorEnhancer::computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double blending) const
{
    const int nbComponents = _sourceGmm.size();
    const int histogramSize = _sourceSample._histogram.size();

    for (int i = 0; i < _wstar.size(); i++)
    {
        double wstar_kj = _wstar[i]._value;
        double k = _wstar[i]._k;
        double j = _wstar[i]._j;
        const MathUtils::VectorNd<3> meanBlending = (1. - blending) * _sourceGmm[k]._mean + blending * _targetGmm[j]._mean;
        const MathUtils::MatrixNd<3>& sigma0 = _sourceGmm[k]._covariance;
        const MathUtils::MatrixNd<3>& sigma1 = _targetGmm[j]._covariance;
        const MathUtils::MatrixNd<3> sigmaBlending = (1. - blending) * MathUtils::MatrixNd<3>::Identity() + blending * sigma0.inverse() * (sigma0 * sigma1).sqrt();

        for (int h = 0, d = 0; h < histogramSize; h++, d += nbComponents)
            colorMap[h] += wstar_kj * _histCompDensities[d + k] * (meanBlending + sigmaBlending * (_sourceSample._histogram[h]._value - _sourceGmm[k]._mean));
    }

    for (int h = 0; h < histogramSize; h++)
    {
        colorMap[h] /= _histDensity[h];
        colorMap[h] = colorMap[h].cwiseMax(0).cwiseMin(255); //clipping
    }

    /*uchar optimalColor = _colorMapping[channel][color];
    double corrBlending = std::min(blending * W1DistTarget / _w1Distance, 1.);
    return (uchar)(corrBlending * (double)optimalColor + (1. - corrBlending) * (double)color);*/
}

//bool ColorEnhancer::findOptimalDistanceCDF(ProbaUtils::CDF& optimalCDF, const ProbaUtils::GMMCDFComponents& components, const ProbaUtils::CDF& targetCDF) const
//{
//    double startConstr[3], endConstr[3];
//    double varMean = 0.;
//    for (int c = 0; c < 3; c++)
//    {
//        startConstr[c] = targetCDF[c][0];
//        endConstr[c] = targetCDF[c][255] - targetCDF[c][254];
//
//        for (int k = 0; k < components[c].size(); k++)
//            varMean += components[c][k]._variance * components[c][k]._weight;
//    }
//    varMean /= 3.;
//    double stdDevMean = sqrt(varMean);
//
//    //Find interval [xMin, xMax] containing narest distance to target distance
//    auto distance = [&](double x)
//        {
//            ProbaUtils::CDF gmmCDF;
//            ProbaUtils::gmmCDF(gmmCDF, components, x, startConstr, endConstr);
//            return abs(ProbaUtils::w1Distance(gmmCDF, targetCDF) - W1DistTarget);
//        };
//
//    const int nbSteps = (int)((StdDevMax - stdDevMean) / StdDevIncr);
//    bool foundInterval = false; 
//    double xMin = -1, xMid = -1, xMax = -1;
//    double dist_xMin = MathUtils::DoubleMax;
//    double dist_xMid = MathUtils::DoubleMax;
//    double dist_xMax = MathUtils::DoubleMax;
//
//    for (int step = 0; step < nbSteps; step++)
//    {
//        xMax = stdDevMean + step * StdDevIncr;
//        xMax = xMax * xMax / varMean;
//        dist_xMax = distance(xMax);
//
//        if (dist_xMax > dist_xMid)
//        {
//            foundInterval = true;
//            break;
//        }
//
//        xMin = xMid;
//        xMid = xMax;
//        dist_xMin = dist_xMid;
//        dist_xMid = dist_xMax;
//    }
//
//    if (!foundInterval)
//        return false;
//
//    if (xMin < 0)
//        xMin = xMid;
//
//    //Golden section search algorithm
//    while ((xMax - xMin) > ConvergenceTol)
//    {
//        double x0 = xMax - (xMax - xMin) / std::numbers::phi;
//        double x1 = xMin + (xMax - xMin) / std::numbers::phi;
//        if (distance(x0) < distance(x1))
//            xMax = x1;
//        else
//            xMin = x0;
//    }
//
//    double xOpt = (xMax + xMin) * 0.5;
//    ProbaUtils::gmmCDF(optimalCDF, components, xOpt, startConstr, endConstr);
//
//    return true;
//}

