#pragma once

#include <opencv2/opencv.hpp>
#include <bitset>


namespace MathUtils
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

    template< typename T>
    inline T clip(T val, T min, T max) { return (val < min) ? min : (val > max) ? max : val; };

    void computeGrayscale(cv::Mat& target, const cv::Mat& source);

    void computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, Filter filter);
    void computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Rect& box, Filter filter);

    void computeImageDHash(const cv::Mat& image, Hash& hash);

    double computeVarianceOfLaplacian(const cv::Mat& image);

    void applyGaussianBlur(uchar* image, const cv::Size& size, double sigma);

    void computeImageBGRFeatures(const cv::Mat& image, const cv::Rect& box, double* features, int featureDiv, int nbFeatures);
    double BGRFeatureDistance(const double* features1, const double* features2, int nbFeatures);

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

