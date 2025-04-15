#pragma once

#include "Photo.h"
#include "ProbaUtils.h"
#include <tuple>


class ColorEnhancer
{
public:
    ColorEnhancer();
    ~ColorEnhancer();

public:
    void computeData(const Photo& photo, const cv::Mat& tile, int mosaicId);
    uchar apply(uchar color, int channel, double blending) const;

private:

private:
    void computeTileCDF(ProbaUtils::CDF& cdf, const cv::Mat& image, const cv::Rect& box) const;

private:
    int _colorMapping[3][256];
};