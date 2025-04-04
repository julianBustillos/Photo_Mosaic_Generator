#pragma once

#include "Photo.h"
#include <tuple>


class PixelAdapter
{
public:
    PixelAdapter(std::tuple<int, int> grid);
    ~PixelAdapter();

public:
    void compute(const Photo& photo);
    void applyCorrection(cv::Mat& tile, double blending, int mosaicId) const;

private:
    struct AdapterData
    {
        double _BGR_cdf[3][256];
    };

private:
    void computeAdapterData(AdapterData& adapterData, const cv::Mat& image, const cv::Rect& box) const;

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<AdapterData> _tileCorrection;
};