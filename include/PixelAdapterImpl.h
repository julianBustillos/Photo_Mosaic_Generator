#pragma once

#include "IPixelAdapter.h"


class PixelAdapterImpl : public IPixelAdapter
{
public:
    PixelAdapterImpl(int subdivisions);
    virtual ~PixelAdapterImpl();

public:
    virtual void compute(const Photo& photo);
    virtual void applyCorrection(cv::Mat& tile, double blending, int mosaicId) const;

private:
    struct AdapterData
    {
        double _BGR_cdf[3][256];
    };

private:
    void computeAdapterData(AdapterData& adapterData, const cv::Mat& image, const cv::Rect& box) const;
private:
    std::vector<AdapterData> _tileCorrection;
};