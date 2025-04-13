#pragma once

#include <opencv2/opencv.hpp>
#include <bitset>


namespace ImageUtils
{
    constexpr int BlurNbBoxes = 3;
    constexpr int HashSize = 8;
    constexpr int HashBits = 2 * HashSize * 8;

    using Hash = std::bitset<HashBits>;

    enum Filter
    {
        AREA,
        BICUBIC,
        LANCZOS
    };

    void resample(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, Filter filter);
    void resample(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Rect& box, Filter filter);

    void computeFeatures(const cv::Mat& image, const cv::Rect& box, double* features, int featureDiv, int nbFeatures);
    double featureDistance(const double* features1, const double* features2, int nbFeatures);

    void gaussianBlur(uchar* image, const cv::Size& size, double sigma);

    void DHash(const cv::Mat& image, Hash& hash);
};

