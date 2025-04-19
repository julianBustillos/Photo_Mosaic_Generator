#pragma once

#include "Photo.h"
#include "ProbaUtils.h"
#include "GaussianMixtureModel.h"
#include <tuple>


class ColorEnhancer
{
public:
    static constexpr double W1DistTarget = 25.6;
    static constexpr int CompoMaxNb = 10;
    static constexpr int MaxIter = 1000;
    static constexpr double ConvergenceTol = 1e-3;
    static constexpr double StdDevIncr = 10.;
    static constexpr double StdDevMax = 12800.;

public:
    ColorEnhancer();
    ~ColorEnhancer();

public:
    void computeData(const Photo& photo, const cv::Mat& tile, int mosaicId);
    uchar apply(uchar color, int channel, double blending) const;

private:
    bool findOptimalDistanceCDF(ProbaUtils::CDF& optimalCDF, const ProbaUtils::GMMCDFComponents& components, const ProbaUtils::CDF& targetCDF) const;

private:
    int _colorMapping[3][256];
    double _w1Distance;
};