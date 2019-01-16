#include "mathTools.h"
#include <opencv2/opencv.hpp>

//DEBUG
#include <iostream>
#include <assert.h>

double cubicInterpolate(double p[4], double x) {
    return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

double bicubicInterpolate(double p[4][4], double x, double y) {
    double arr[4];
    arr[0] = cubicInterpolate(p[0], y);
    arr[1] = cubicInterpolate(p[1], y);
    arr[2] = cubicInterpolate(p[2], y);
    arr[3] = cubicInterpolate(p[3], y);
    return cubicInterpolate(arr, x);
}
//DEBUG

 const double MathTools::_biCubicCoeffs[16] = { -1, 3, -3, 1, 2, -5, 4, -1, -1, 0, 1, 0, 0, 2, 0, 0 };


 int MathTools::clipInt(int val, int min, int max)
 {
     return (val < min) ? min : (val >= max) ? max : val;
 }

 uchar MathTools::biCubicInterpolation(double x, double y, const cv::Mat &pointColorGrid)
{
    const cv::Mat biCubicMat(4, 4, CV_64F, (void *)_biCubicCoeffs);
    cv::Mat vx(4, 1, CV_64F);
    cv::Mat vy(4, 1, CV_64F);
    double *vxData = (double *)vx.data;
    double *vyData = (double *)vy.data;

    vxData[3] = vyData[3] = 1.;
    for (int i = 2; i > -1; i--) {
        vxData[i] = vxData[i + 1] * x;
        vyData[i] = vyData[i + 1] * y;
    }

    cv::Mat interpolation = vx.t() * biCubicMat * pointColorGrid.t() * biCubicMat.t() * vy;
    return (uchar)clipInt((int)floor(((double *)interpolation.data)[0] / 4.), 0, 256);
}
