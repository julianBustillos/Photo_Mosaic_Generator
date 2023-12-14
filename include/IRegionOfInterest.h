#pragma once

#include <opencv2/opencv.hpp>


class IRegionOfInterest
{
public:
    //TODO : ADD COMMENT
    IRegionOfInterest() {};

    //TODO : ADD COMMENT
    virtual ~IRegionOfInterest() {};

    //TODO : ADD COMMENT
    virtual void find(const cv::Mat& image, cv::Point& firstPixel, const cv::Size& cropSize, bool rowDirSearch) const = 0;
};