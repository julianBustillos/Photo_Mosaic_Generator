#pragma once

#include "Photo.h"
#include <opencv2/opencv.hpp>


class IPixelAdapter
{
public:
    IPixelAdapter(int subdivisions) : _subdivisions(subdivisions) {};
    virtual ~IPixelAdapter() {};

public:
    virtual void compute(const Photo& photo) = 0;
    virtual void applyCorrection(cv::Mat& tile, double blending, int mosaicId) const = 0;

protected:
    const int _subdivisions;
};
