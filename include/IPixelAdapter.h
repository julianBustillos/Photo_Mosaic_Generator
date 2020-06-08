#pragma once

#include "Photo.h"
#include <opencv2/opencv.hpp>


class IPixelAdapter
{
public:
    //TODO : ADD COMMENT
    IPixelAdapter(const Photo &photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};

    //TODO : ADD COMMENT
    virtual ~IPixelAdapter() {};

    //TODO : ADD COMMENT
    virtual void compute() = 0;

    //TODO : ADD COMMENT
    virtual void applyCorrection(cv::Mat &tile, int mosaicId) const = 0;

protected:
    const Photo &_photo;
    const int _subdivisions;
};
