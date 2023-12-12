#pragma once

#include "IPixelAdapter.h"


class PixelAdapterImpl : public IPixelAdapter
{
public:
    PixelAdapterImpl(const Photo& photo, int subdivisions) : IPixelAdapter(photo, subdivisions) {};
    virtual ~PixelAdapterImpl() {};
    virtual void compute();
    virtual void applyCorrection(cv::Mat& tile, int mosaicId) const;

private:
    struct AdapterData
    {
        double _BGR_cdf[3][256];
    };

private:
    void computeAdapterData(AdapterData& adapterData, const uchar* data, const cv::Point& firstPixel, const cv::Size& size, int step) const;
private:
    std::vector<AdapterData> _tileCorrection;
};