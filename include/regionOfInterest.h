#pragma once

#include "opencv2/opencv.hpp"


class RegionOfInterest {
public:
    RegionOfInterest(const cv::Mat &image);
    ~RegionOfInterest() {};
    //void getROIBinaryImage(std::vector<bool> &ROIBinaryImage);

//private:
    //std::vector<bool> _ROIBinaryImage;
};