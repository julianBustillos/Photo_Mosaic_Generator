#pragma once

#include "IPixelAdapter.h"


class PixelAdapterImpl : public IPixelAdapter
{
private:
    static constexpr double HistogramCorrectionBlending = 0.4;

public:
    PixelAdapterImpl(std::shared_ptr<const Photo> photo, int subdivisions);
    virtual ~PixelAdapterImpl();

public:
    virtual void compute();
    virtual void applyCorrection(cv::Mat& tile, int mosaicId) const;

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