#pragma once

#include "photo.h"
#include <opencv2/opencv.hpp>


class PixelAdapter {
public:
    PixelAdapter(Photo photo, int subdivisions);
    ~PixelAdapter() {};
    void applyCorrection(cv::Mat &tile, int mosaicId) const;

private:
    struct AdapterData {
        double _BGR_cdf[3][256];
    };

private:
    void computeAdapterData(AdapterData &adapterData, const uchar *data, const cv::Point &firstPixel, const cv::Size &size, int step) const;
private:
    std::vector<AdapterData> _tileCorrection;
};
