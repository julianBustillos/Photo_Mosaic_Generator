#pragma once

#include "IRegionOfInterest.h"
#include <opencv2/objdetect.hpp>
#include <string>


class FaceDetectionROIImpl : public IRegionOfInterest
{
private:
    static constexpr double MinCroppedRatio = 0.9;
    static constexpr double HighFaceConfidence = 0.8;
    static constexpr double LowFaceConfidence = 0.5;

public:
    FaceDetectionROIImpl();
    ~FaceDetectionROIImpl();

public:
    virtual void find(const cv::Mat& image, cv::Rect& box, bool rowDirSearch) const;
    virtual void initialize();

private:
    void getDetectionROI(const cv::Size& imageSize, const cv::Mat& faces, cv::Rect& box, double scaleInv, bool rowDirSearch) const;
    void getDefaultROI(const cv::Size& imageSize, cv::Rect& box, bool rowDirSearch) const;

private:
    void dumpDetection(std::string path, const cv::Mat& image, const cv::Mat& faces, double scaleInv, bool confidence) const;

private:
    const static int _detectionSize;

private:
    cv::Ptr<cv::FaceDetectorYN> _faceDetector;
};