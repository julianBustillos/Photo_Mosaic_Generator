#pragma once

#include <opencv2/opencv.hpp>


class IRegionOfInterest
{
public:
    IRegionOfInterest() {};
    virtual ~IRegionOfInterest() {};
    virtual void find(const cv::Mat &image) const = 0;
};