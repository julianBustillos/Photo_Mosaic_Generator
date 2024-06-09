#pragma once

#include "Photo.h"
#include <opencv2/opencv.hpp>


class IPixelAdapter
{
public:
    IPixelAdapter(std::shared_ptr<const Photo> photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};
    virtual ~IPixelAdapter() { _photo.reset(); };

public:
    virtual void compute() = 0;
    virtual void applyCorrection(cv::Mat& tile, int mosaicId) const = 0;

protected:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};
