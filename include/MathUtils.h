#pragma once

#include <opencv2/opencv.hpp>
#include <bitset>


namespace MathUtils
{
    constexpr int BlurNbBoxes = 3;
    constexpr int HashSize = 8;
    constexpr int HashBits = 2 * HashSize * 8;

    using Hash = std::bitset<HashBits>;


    template< typename T>
    inline T clip(T val, T min, T max);

    inline int getDataIndex(int i, int j, int c, int step);
    inline int getClippedDataIndex(int i, int j, int step, const cv::Size& size);

    void computeGrayscale(cv::Mat& target, const cv::Mat& source);
    void computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Point& cropFirstPixel, const cv::Size& cropSize);
    void computeImageDHash(const cv::Mat& image, Hash& hash);
    void applyGaussianBlur(uchar* image, const cv::Size& size, double sigma);
    void computeImageBGRFeatures(const uchar* image, const cv::Size& size, const cv::Point& firstPos, int step, double* features, int featureDirSubdivision);
    double BGRFeatureDistance(const double* vec1, const double* vec2, int size);
    void convertBGRtoHSV(double& hue, double& saturation, double& value, uchar blue, uchar green, uchar red);
    void convertHSVtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double value);
    void convertBGRtoHSL(double& hue, double& saturation, double& lightness, uchar blue, uchar green, uchar red);
    void convertHSLtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double lightness);
    void convertBGRtoHSI(double& hue, double& saturation, double& intensity, uchar blue, uchar green, uchar red);
    void convertHSItoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double intensity);
    void convertBGRtoLUV(double& L, double& u, double& v, uchar blue, uchar green, uchar red);
    void convertLUVtoBGR(uchar& blue, uchar& green, uchar& red, double L, double u, double v);
    void convertBGRtoLAB(double& L, double& a, double& b, uchar blue, uchar green, uchar red);
    void convertLABtoBGR(uchar& blue, uchar& green, uchar& red, double L, double a, double b);
};

template<typename T>
inline T MathUtils::clip(T val, T min, T max)
{
    return (val < min)?min:(val > max)?max:val;
}


inline int MathUtils::getDataIndex(int i, int j, int c, int step)
{
    return c * (i * step + j);
}

inline int MathUtils::getClippedDataIndex(int i, int j, int step, const cv::Size& size)
{
    int iSafe = MathUtils::clip<int>(i, 0, size.height - 1);
    int jSafe = MathUtils::clip<int>(j, 0, size.width - 1);
    return 3 * (iSafe * (step?step:size.width) + jSafe);
}