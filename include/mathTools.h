#pragma once

#include <opencv2/opencv.hpp>

class MathTools {
public:
    static int clipInt(int val, int min, int max);
    static uchar biCubicInterpolation(double x, double y, const double *pixelColorGrid);

private:
    MathTools() {};
    ~MathTools() {};

private:
    static const double _biCubicCoeffs[16];
};