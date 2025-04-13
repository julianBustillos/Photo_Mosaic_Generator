#pragma once

#include <opencv2/opencv.hpp>


namespace ColorUtils
{
    template< typename T>
    inline T clip(T val, T min, T max) { return (val < min) ? min : (val > max) ? max : val; };

    void BGRtoHSV(double& hue, double& saturation, double& value, uchar blue, uchar green, uchar red);
    void HSVtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double value);
    void BGRtoHSL(double& hue, double& saturation, double& lightness, uchar blue, uchar green, uchar red);
    void HSLtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double lightness);
    void BGRtoHSI(double& hue, double& saturation, double& intensity, uchar blue, uchar green, uchar red);
    void HSItoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double intensity);
    void BGRtoLUV(double& L, double& u, double& v, uchar blue, uchar green, uchar red);
    void LUVtoBGR(uchar& blue, uchar& green, uchar& red, double L, double u, double v);
    void BGRtoLAB(double& L, double& a, double& b, uchar blue, uchar green, uchar red);
    void LABtoBGR(uchar& blue, uchar& green, uchar& red, double L, double a, double b);
};

