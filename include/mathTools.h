#pragma once

#include <opencv2/opencv.hpp>

class MathTools {
public:
    static int clipInt(int val, int min, int max);
    static uchar biCubicInterpolation(double x, double y, const cv::Mat &pointColorGrid);

private:
    MathTools() {};
    ~MathTools() {};

private:
    static const double _biCubicCoeffs[16];
};