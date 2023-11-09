#pragma once

#include <opencv2/opencv.hpp>


class RegionOfInterest 
{
public:
    RegionOfInterest(const cv::Mat &image, cv::Point & firstPixelPos, const cv::Size &cropSize, bool rowDirSearch);
    ~RegionOfInterest() {};

private:
    void getDefaultROI(const cv::Size &imageSize, cv::Point & firstPixelPos, const cv::Size &cropSize, bool rowDirSearch);
    void findRowROI(const cv::Size &imageSize, const std::vector<int>& clusterMapping, const std::vector<double> &saliency, int jMean, cv::Point & firstPixelPos, const cv::Size &cropSize);
    void findColROI(const cv::Size &imageSize, const std::vector<int>& clusterMapping, const std::vector<double> &saliency, int iMean, cv::Point & firstPixelPos, const cv::Size &cropSize);
};