#pragma once

#include "Photo.h"
#include <tuple>


class ColorEnhancer
{
public:
    ColorEnhancer(std::tuple<int, int> grid);
    ~ColorEnhancer();

public:
    void computeData(const Photo& photo);
    void apply(cv::Mat& tile, double blending, int mosaicId) const;

private:
    struct EnhancerData
    {
        double _colorCDF[3][256];
    };

private:
    void computeTileData(EnhancerData& data, const cv::Mat& image, const cv::Rect& box) const;

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<EnhancerData> _tileData;
};