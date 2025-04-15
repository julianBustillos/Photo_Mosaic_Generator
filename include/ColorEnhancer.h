#pragma once

#include "Photo.h"
#include "ProbaUtils.h"
#include <tuple>


class ColorEnhancer
{
public:
    ColorEnhancer(int gridWidth, int gridHeight);
    ~ColorEnhancer();

public:
    void computeData(const Photo& photo);
    void apply(cv::Mat& tile, double blending, int mosaicId) const;

private:

private:
    void computeTileCDF(ProbaUtils::CDF& cdf, const cv::Mat& image, const cv::Rect& box) const;

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<ProbaUtils::CDF> _tileCDF;
};