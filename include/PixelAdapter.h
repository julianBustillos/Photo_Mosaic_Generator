#pragma once

#include "Photo.h"


class PixelAdapter
{
public:
    PixelAdapter(int subdivisions);
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
    const int _subdivisions;
    std::vector<AdapterData> _tileCorrection;
};