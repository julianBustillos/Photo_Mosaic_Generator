#pragma once

#include "IRegionOfInterest.h"
#include <opencv2/objdetect.hpp>
#include <string>


class FaceDetectionROIImpl : public IRegionOfInterest
{
public:
    FaceDetectionROIImpl();
    ~FaceDetectionROIImpl();
    virtual void find(const cv::Mat& image, cv::Point& firstPixel, const cv::Size& cropSize, bool rowDirSearch) const;

private:
    std::string getCurrentProcessDirectory();
    void getDetectionROI(const cv::Size& imageSize, const cv::Mat& faces, cv::Point& firstPixel, const cv::Size& cropSize, double scaleInv, bool rowDirSearch) const;
    void getDefaultROI(const cv::Size& imageSize, cv::Point& firstPixel, const cv::Size& cropSize, bool rowDirSearch) const;

private:
    void dumpDetection(std::string path, const cv::Mat& image, const cv::Mat& faces, double scaleInv, bool confidence) const;

private:
    const static int _detectionSize;

private:
    cv::Ptr<cv::FaceDetectorYN> _faceDetector;
};