#include "mathTools.h"
#include <opencv2/opencv.hpp>


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