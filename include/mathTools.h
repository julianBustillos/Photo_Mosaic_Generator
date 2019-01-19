#pragma once

#include <opencv2/opencv.hpp>

class MathTools {
public:
    static int clipInt(int val, int min, int max);
    static uchar biCubicInterpolation(double x, double y, const uchar *pixelColorGrid);
    static void computeTileFeatures(uchar *image, int tileWidth, int tileHeight, int iFirstPos, int jFirstPos, int step, int channels, double *features);

private:
    MathTools() {};
    ~MathTools() {};

private:
    static const double _biCubicCoeffs[16];
};