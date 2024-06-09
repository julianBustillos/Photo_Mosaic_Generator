#pragma once

#include <opencv2/opencv.hpp>


class IRegionOfInterest
{
public:
    IRegionOfInterest() {};
    virtual ~IRegionOfInterest() {};

public:
    virtual void initialize() = 0;
    virtual void find(const cv::Mat& image, cv::Rect& box, bool rowDirSearch) const = 0;
};