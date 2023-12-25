#pragma once

#include "Photo.h"
#include <opencv2/opencv.hpp>


class IPixelAdapter
{
public:
    //TODO : ADD COMMENT
    IPixelAdapter(std::shared_ptr<const Photo> photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};

    //TODO : ADD COMMENT
    virtual ~IPixelAdapter() { _photo.reset(); };

    //TODO : ADD COMMENT
    virtual void compute() = 0;

    //TODO : ADD COMMENT
    virtual void applyCorrection(cv::Mat& tile, int mosaicId) const = 0;

protected:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};
