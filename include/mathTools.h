#pragma once

#include <opencv2/opencv.hpp>

# define M_PI           3.14159265358979323846  /* pi */


class MathTools {
public:
    template< typename T>
    static T clip(T val, T min, T max);

public:
    static uchar biCubicInterpolation(double x, double y, const uchar *pixelColorGrid);
    static void computeImageFeatures(const uchar *image, int width, int height, int iFirstPos, int jFirstPos, int step, int channels, double *features, int featureDirSubdivision);
    static double squareDistance(const double *vec1, const double *vec2, int size);
    static void convertBGRtoHSV(double &hue, double &saturation, double &value, uchar blue, uchar green, uchar red);
    static void convertHSVtoBGR(uchar &blue, uchar &green, uchar &red, double hue, double saturation, double value);
    static void convertBGRtoHSL(double &hue, double &saturation, double &lightness, uchar blue, uchar green, uchar red);
    static void convertHSLtoBGR(uchar &blue, uchar &green, uchar &red, double hue, double saturation, double lightness);
    static void convertBGRtoHSI(double &hue, double &saturation, double &intensity, uchar blue, uchar green, uchar red);
    static void convertHSItoBGR(uchar &blue, uchar &green, uchar &red, double hue, double saturation, double intensity);

private:
    MathTools() {};
    ~MathTools() {};

private:
    static const double _biCubicCoeffs[16];
};

template<typename T>
inline T MathTools::clip(T val, T min, T max)
{
    return (val < min) ? min : (val > max) ? max : val;
}
