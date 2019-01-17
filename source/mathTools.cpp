#include "mathTools.h"
#include <opencv2/opencv.hpp>


const double MathTools::_biCubicCoeffs[16] = { -1, 2, -1, 0, 3, -5, 0, 2, -3, 4, 1, 0, 1, -1, 0, 0 };

 int MathTools::clipInt(int val, int min, int max)
 {
     return (val < min) ? min : (val > max) ? max : val;
 }

 uchar MathTools::biCubicInterpolation(double x, double y, const double *pixelColorGrid)
{
     /*
     Compute bicubic interpolation :
     vx = [x^3, x^2, x, 1]
     vy = [y^3, y^2, y, 1]
     B : 4*4 bicubic coeff matrix
     P : 4*4 pixel grid value matrix
     interpolation = 1/4 * vx * B * P^t * B^t * vy
     */

     double vx[4], vy[4], vxCoeffsMult[4], vyCoeffsMult[4];

     vx[3] = vy[3] = 1.;
     for (int i = 2; i > -1; i--) {
         vx[i] = vx[i + 1] * x;
         vy[i] = vy[i + 1] * y;
     }

     double xTemp, yTemp;
     for (int j = 0; j < 4; j++) {
         xTemp = yTemp = 0;
         for (int i = 0; i < 4; i++) {
             xTemp += vx[i] * _biCubicCoeffs[j * 4 + i];
             yTemp += vy[i] * _biCubicCoeffs[j * 4 + i];
         }
         vxCoeffsMult[j] = xTemp;
         vyCoeffsMult[j] = yTemp;
     }

     double interpolation = 0;
     for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
             interpolation += vxCoeffsMult[i] * pixelColorGrid[i * 4 + j] * vyCoeffsMult[j];
         }
     }
     interpolation /= 4.;

     return (uchar)clipInt((int)floor(interpolation), 0, 255);
}
