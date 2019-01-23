#pragma once

#include <opencv2/opencv.hpp>

class MathTools {
public:
    static int clipInt(int val, int min, int max);
    static uchar biCubicInterpolation(double x, double y, const uchar *pixelColorGrid);
    static void computeImageFeatures(const uchar *image, int width, int height, int iFirstPos, int jFirstPos, int step, int channels, double *features);
    static double squareDistance(const double *vec1, const double *vec2, int size);

private:
    MathTools() {};
    ~MathTools() {};

private:
    static const double _biCubicCoeffs[16];
};