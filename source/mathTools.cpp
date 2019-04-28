#include "mathTools.h"


const double MathTools::_biCubicCoeffs[16] = { -1, 2, -1, 0, 3, -5, 0, 2, -3, 4, 1, 0, 1, -1, 0, 0 };


 uchar MathTools::biCubicInterpolation(double x, double y, const uchar *pixelColorGrid)
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

     return (uchar)clip<int>((int)round(interpolation), 0, 255);
}

 void MathTools::applyGaussianBlur(uchar * image, const cv::Size & size, int sigma, int nbBoxes)
 {
     uchar *temp = new uchar[3 * size.width * size.height];
     if (!temp)
         return;

     std::vector<int> boxRadius(nbBoxes);
     getGaussianApproximationBoxRadiuses(sigma, boxRadius);

     if (boxRadius[boxRadius.size() - 1] <= std::min(size.width, size.height) / 2) {
         for (int k = 0; k < boxRadius.size(); k++)
             applyBoxBlur(image, temp, size, boxRadius[k]);
     }

     delete temp;
 }

 void MathTools::computeImageBGRFeatures(const uchar * image, const cv::Size&size, const cv::Point &firstPos, int step, double * features, int featureDirSubdivision)
 {
     int blockWidth  = (int)ceil(size.width / (double)featureDirSubdivision);
     int blockHeight = (int)ceil(size.height / (double)featureDirSubdivision);
     
     for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++)
         features[k] = 0;

     for (int i = 0; i < size.height; i++) {
         for (int j = 0; j < size.width; j++) {
             int blockId = featureDirSubdivision * (i / blockHeight) + j / blockWidth;
             int imageId = getDataIndex(firstPos.y + i, firstPos.x + j, step);
             for (int c = 0; c < 3; c++) 
                 features[3 * blockId + c] += image[imageId + c];
         }
     }

     for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++) {
         int corrBlockHeight = (k < featureDirSubdivision * (featureDirSubdivision - 1) * 3) ? blockHeight : size.height - (featureDirSubdivision - 1) * blockHeight;
         int corrBlockWidth  = (((k / 4 + 1) % featureDirSubdivision) != 0) ? blockWidth : size.width - (featureDirSubdivision - 1) * blockWidth;
         features[k] /= corrBlockWidth * corrBlockHeight;
     }
 }

 double MathTools::BGRFeatureDistance(const double * vec1, const double * vec2, int size)
 {
     //Use deltaE distance
     double sumDist = 0.;
     for (int i = 0; i < 3 * size; i += 3) {
         double dB = vec1[i] - vec2[i];
         double dG = vec1[i + 1] - vec2[i + 1];
         double dR = vec1[i + 2] - vec2[i + 2];
         double mR = (vec1[i + 2] + vec2[i + 2]) / 2.;
         double sqDist = (2. + mR / 256.) * dR * dR + 4 * dG * dG + (2. + (255. - mR) / 256.) * dB * dB;
         sumDist += sqrt(sqDist);
     }

     return sumDist;
 }

 void MathTools::convertBGRtoHSV(double & hue, double & saturation, double & value, uchar blue, uchar green, uchar red)
 {
     double B = blue / 255.;
     double G = green / 255.;
     double R = red / 255.;

     double min = B, max = B;
     min = std::min(min, G);
     min = std::min(min, R);
     max = std::max(max, G);
     max = std::max(max, R);

     if (min == max)
         hue = 0.;
     else if (max == B)
         hue = M_PI / 3. * (4 + (R - G) / (max - min));
     else if (max == G)
         hue = M_PI / 3. * (2 + (B - R) / (max - min));
     else
         hue = M_PI / 3. * (G - B) / (max - min);

     if (hue < 0.)
         hue += 2 * M_PI;

     if (max == 0.)
         saturation = 0.;
     else
         saturation = (max - min) / max;

     value = max;

     hue = clip<double>(hue, 0., 2 * M_PI);
     saturation = clip<double>(saturation, 0., 1.);
     value = clip<double>(value, 0., 1.);
 }

 void MathTools::convertHSVtoBGR(uchar & blue, uchar & green, uchar & red, double hue, double saturation, double value)
 {
     double C = value * saturation;
     double H = hue * 3. / M_PI;
     double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

     double B, G, R;
     if (H < 0. || 6. < H) {
         B = 0;
         G = 0;
         R = 0;
     }
     else if (H <= 1.) {
         B = 0.; 
         G = X; 
         R = C;
     }
     else if (H <= 2.) {
         B = 0.; 
         G = C; 
         R = X;
     }
     else if (H <= 3.) {
         B = X; 
         G = C; 
         R = 0.;
     }
     else if (H <= 4.) {
         B = C; 
         G = X; 
         R = 0.;
     }
     else if (H <= 5.) {
         B = C; 
         G = 0.; 
         R = X;
     }
     else if (H <= 6.) {
         B = X; 
         G = 0.;
         R = C;
     }

     double m = value - C;

     blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
     green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
     red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
 }

 void MathTools::convertBGRtoHSL(double & hue, double & saturation, double & lightness, uchar blue, uchar green, uchar red)
 {
     double B = blue / 255.;
     double G = green / 255.;
     double R = red / 255.;

     double min = B, max = B;
     min = std::min(min, G);
     min = std::min(min, R);
     max = std::max(max, G);
     max = std::max(max, R);

     if (min == max)
         hue = 0.;
     else if (max == B)
         hue = M_PI / 3. * (4 + (R - G) / (max - min));
     else if (max == G)
         hue = M_PI / 3. * (2 + (B - R) / (max - min));
     else
         hue = M_PI / 3. * (G - B) / (max - min);

     if (hue < 0.)
         hue += 2 * M_PI;

     lightness = (max + min) / 2.;

     if (max == 0. || min == 1.)
         saturation = 0.;
     else
         saturation = (max - lightness) / (std::min(lightness, 1. - lightness));

     hue = clip<double>(hue, 0., 2 * M_PI);
     saturation = clip<double>(saturation, 0., 1.);
     lightness = clip<double>(lightness, 0., 1.);
 }

 void MathTools::convertHSLtoBGR(uchar & blue, uchar & green, uchar & red, double hue, double saturation, double lightness)
 {
     double C = (1. - std::abs(2. * lightness - 1.)) * saturation;
     double H = hue * 3. / M_PI;
     double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

     double B, G, R;
     if (H < 0. || 6. < H) {
         B = 0;
         G = 0;
         R = 0;
     }
     else if (H <= 1.) {
         B = 0.;
         G = X;
         R = C;
     }
     else if (H <= 2.) {
         B = 0.;
         G = C;
         R = X;
     }
     else if (H <= 3.) {
         B = X;
         G = C;
         R = 0.;
     }
     else if (H <= 4.) {
         B = C;
         G = X;
         R = 0.;
     }
     else if (H <= 5.) {
         B = C;
         G = 0.;
         R = X;
     }
     else if (H <= 6.) {
         B = X;
         G = 0.;
         R = C;
     }

     double m = lightness - C / 2.;

     blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
     green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
     red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
 }

 void MathTools::convertBGRtoHSI(double & hue, double & saturation, double & intensity, uchar blue, uchar green, uchar red)
 {
     double B = blue / 255.;
     double G = green / 255.;
     double R = red / 255.;
     double i = B + G + R;
     intensity = i / 3;

     if (R == G && G == B) {
         hue = 0.;
         saturation = 0.;
     }
     else {
         double w = 0.5 * (2 * R - G - B) / sqrt((R - G) * (R - G) + (R - B) * (G - B));
         w = clip<double>(w, -1., 1.);
         hue = acos(w);
         if (B > G)
             hue = 2 * M_PI - hue;
         double min;
         if (R <= B && R <= G)
             min = R;
         else if (B <= G)
             min = B;
         else
             min = G;
         saturation = 1 - 3 * min / i;
     }

     hue = clip<double>(hue, 0., 2 * M_PI);
     saturation = clip<double>(saturation, 0., 1.);
     intensity = clip<double>(intensity, 0., 1.);
 }

 void MathTools::convertHSItoBGR(uchar & blue, uchar & green, uchar & red, double hue, double saturation, double intensity)
 {
     double B, G, R;

     if (saturation == 0.)
         B = G = R = intensity;
     else {
         if ((hue >= 0.) && (hue < 2. * M_PI / 3.)) {
             B = (1. - saturation) / 3.;
             R = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
             G = 1. - R - B;
         }
         else if ((hue >= 2. * M_PI / 3.) && (hue < 4. * M_PI / 3.)) {
             hue = hue - 2. * M_PI / 3.;
             R = (1. - saturation) / 3.;
             G = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
             B = 1. - R - G;
         }
         else if ((hue >= 4. * M_PI / 3.) && (hue < 2. * M_PI)) {
             hue = hue - 4. * M_PI / 3.;
             G = (1. - saturation) / 3.;
             B = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
             R = 1. - B - G;
         }
         else {
             B = G = R = 0.;
         }

         B *= 3 * intensity;
         G *= 3 * intensity;
         R *= 3 * intensity;
     }

     blue = (uchar)clip<int>((int)round(B * 255.), 0, 255);
     green = (uchar)clip<int>((int)round(G * 255.), 0, 255);
     red = (uchar)clip<int>((int)round(R * 255.), 0, 255);
 }

 void MathTools::getGaussianApproximationBoxRadiuses(int sigma, std::vector<int> &boxRadius)
 {
     int n = (int)boxRadius.size();
     double wIdeal = sqrt(12. * sigma * sigma / n + 1.);
     int wl = (int)floor(wIdeal);
     if ((wl % 2) == 0)
         wl--;
     int wu = wl + 2;
     int m = (int)round((12. * sigma * sigma - n * wl * wl - 4. * n * wl - 3 * n) / (-4. * wl - 4.));
     int wlr = (wl - 1) / 2;
     int wur = (wu - 1) / 2;

     for (int k = 0; k < n; k++)
         boxRadius[k] = (k < m) ? wlr : wur;
 }

 void MathTools::applyBoxBlur(uchar * image, uchar * temp, const cv::Size & size, int boxRadius)
 {
     applyRowBlur(image, temp, size, boxRadius);
     applyColBlur(temp, image, size, boxRadius);
 }

 void MathTools::applyRowBlur(uchar * source, uchar * target, const cv::Size & size, int lineRadius)
 {
     int lineSize = 2 * lineRadius + 1;
     int accumulator[3];
     int start[3];
     int end[3];

     for (int i = 0; i < size.height; i++) {
         // Average filter start middle and end indices
         int startId = 3 * i * size.width;
         int endId = 3 * (i * size.width + lineRadius);
         int targetId = startId;

         // Initialization 
         for (int c = 0; c < 3; c++) {
             start[c] = source[startId + c];
             end[c] = source[3 * (i * size.width + (size.height - 1)) + c];
             accumulator[c] = start[c] * (lineRadius + 1);
         }

         for (int j = 0; j < lineRadius; j++) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[startId + 3 * j + c];
             }
         }

         // Average filtering
         for (int j = 0; j <= lineRadius; j++, targetId += 3, endId += 3) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[endId + c] - start[c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }

         for (int j = lineRadius + 1; j < size.width - lineRadius; j++, targetId += 3, startId += 3, endId += 3) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[endId + c] - source[startId + c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }

         for (int j = size.width - lineRadius; j < size.width; j++, targetId += 3, startId += 3) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += end[c] - source[startId + c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }
     }
 }

 void MathTools::applyColBlur(uchar * source, uchar * target, const cv::Size & size, int lineRadius)
 {
     int lineSize = 2 * lineRadius + 1;
     int accumulator[3];
     int start[3];
     int end[3];

     for (int j = 0; j < size.width; j++) {
         // Average filter start middle and end indices
         int startId = 3 * j;
         int endId = 3 * (lineRadius * size.width + j);
         int targetId = startId;

         // Initialization 
         for (int c = 0; c < 3; c++) {
             start[c] = source[startId + c];
             end[c] = source[3 * ((size.height - 1) * size.width + j) + c];
             accumulator[c] = start[c] * (lineRadius + 1);
         }

         for (int i = 0; i < lineRadius; i++) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[startId + 3 * i * size.width + c];
             }
         }

         // Average filtering
         for (int i = 0; i <= lineRadius; i++, targetId += 3 * size.width, endId += 3 * size.width) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[endId + c] - start[c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }
         
         for (int i = lineRadius + 1; i < size.height - lineRadius; i++, targetId += 3 * size.width, startId += 3 * size.width, endId += 3 * size.width) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += source[endId + c] - source[startId + c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }

         for (int i = size.height - lineRadius; i < size.height; i++, targetId += 3 * size.width, startId += 3 * size.width) {
             for (int c = 0; c < 3; c++) {
                 accumulator[c] += end[c] - source[startId + c];
                 target[targetId + c] = (uchar)round(accumulator[c] / lineSize);
             }
         }
     }
 }
