#pragma once

#include "IRegionOfInterest.h"
#include <opencv2/opencv.hpp>


class MeanSHiftROIImpl : public IRegionOfInterest
{
public:
    MeanSHiftROIImpl();
    ~MeanSHiftROIImpl() {};
    virtual void find(const cv::Mat& image, cv::Point& firstPixel, const cv::Size& cropSize, bool rowDirSearch) const;

private:
    void getDefaultROI(const cv::Size& imageSize, cv::Point& firstPixel, const cv::Size& cropSize, bool rowDirSearch) const;
    void findRowROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int jMean, cv::Point& firstPixel, const cv::Size& cropSize) const;
    void findColROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int iMean, cv::Point& firstPixel, const cv::Size& cropSize) const;
};