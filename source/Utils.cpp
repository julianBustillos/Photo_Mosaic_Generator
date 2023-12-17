#include "Utils.h"
#include "variables.h"


/*
    Unnamed namespace methods called by Utils methods
*/

namespace
{
    double cubeRoot(double t)
    {
        double inv3 = 1. / 3.;
        double root = 1.4774329094 - 0.8414323527 / (t + 0.7387320679);

        while (abs(root * root * root - t) > 0.000001)
        {
            root = (2. * root + t / (root * root)) * inv3;
        }

        return root;
    }

    void computeAreaInterpolationPixel(uchar* pixel, const uchar* source, const cv::Size& size, int i, int j, double scaleInv)
    {
        /*
        Compute area interpolation :
        Compute convolution matrix representing pixel weights area based on original image subdivision according to new size.
        */

        double xMin = (double)j * scaleInv;
        double xMax = (double)(j + 1) * scaleInv;
        double yMin = (double)i * scaleInv;
        double yMax = (double)(i + 1) * scaleInv;

        int xLoBound = (int)std::floor(xMin);
        int xUpBound = (int)std::ceil(xMax);
        int yLoBound = (int)std::floor(yMin);
        int yUpBound = (int)std::ceil(yMax);

        for (int color = 0; color < 3; color++)
        {
            double colorVal = 0.;
            double coefSum = 0.;
            for (int col = xLoBound; col <= xUpBound; col++)
            {
                double colCoef = 1.;
                if (col == xLoBound)
                {
                    colCoef = (double)(xLoBound + 1) - xMin;
                }
                else if (col == xUpBound)
                {
                    colCoef = xMax - (double)(xMax - 1);
                }
                for (int row = yLoBound; row <= yUpBound; row++)
                {
                    double rowCoef = 1.;
                    if (row == yLoBound)
                    {
                        rowCoef = (double)(yLoBound + 1) - yMin;
                    }
                    else if (row == yUpBound)
                    {
                        rowCoef = yMax - (double)(yMax - 1);
                    }
                    int dataId = Utils::getClippedDataIndex(row, col, size.width, size);
                    colorVal += (double)source[dataId + color] * rowCoef * colCoef;
                    coefSum += rowCoef * colCoef;
                }
            }

            pixel[color] = (uchar)Utils::clip<int>((int)round(colorVal / coefSum), 0, 255);
        }
    }

    uchar biCubicInterpolation(double x, double y, const uchar* pixelGrid)
    {
        /*
        Compute bicubic interpolation :
        vx = [x^3, x^2, x, 1]
        vy = [y^3, y^2, y, 1]
        B : 4*4 bicubic coeff matrix
        P : 4*4 pixel grid value matrix
        interpolation = 1/4 * vx * B * P^t * B^t * vy
        */
        static const double biCubicCoeffs[16] = {-1,  2, -1,  0,
                                                  3, -5,  0,  2,
                                                 -3,  4,  1,  0,
                                                  1, -1,  0,  0};


        double vx[4], vy[4], vxCoeffsMult[4], vyCoeffsMult[4];

        vx[3] = vy[3] = 1.;
        for (int i = 2; i > -1; i--)
        {
            vx[i] = vx[i + 1] * x;
            vy[i] = vy[i + 1] * y;
        }

        double xTemp, yTemp;
        for (int j = 0; j < 4; j++)
        {
            xTemp = yTemp = 0;
            for (int i = 0; i < 4; i++)
            {
                xTemp += vx[i] * biCubicCoeffs[j * 4 + i];
                yTemp += vy[i] * biCubicCoeffs[j * 4 + i];
            }
            vxCoeffsMult[j] = xTemp;
            vyCoeffsMult[j] = yTemp;
        }

        double interpolation = 0;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                interpolation += vxCoeffsMult[i] * pixelGrid[i * 4 + j] * vyCoeffsMult[j];
            }
        }
        interpolation /= 4.;

        return (uchar)Utils::clip<int>((int)round(interpolation), 0, 255);
    }

    void computeBicubicInterpolationPixel(uchar* pixel, const uchar* source, const cv::Size& size, int i, int j, double scaleInv)
    {
        uchar pixelGrid[16];
        double x = (double)j * scaleInv;
        double y = (double)i * scaleInv;
        int iFirstGrid = (int)round(y) - 2;
        int jFirstGrid = (int)round(x) - 2;

        for (int color = 0; color < 3; color++)
        {
            for (int col = 0; col < 4; col++)
            {
                for (int row = 0; row < 4; row++)
                {
                    int dataId = Utils::getClippedDataIndex(iFirstGrid + row, jFirstGrid + col, size.width, size);
                    pixelGrid[col * 4 + row] = source[dataId + color];
                }
            }

            pixel[color] = biCubicInterpolation(x - round(x) + 1, y - round(y) + 1, pixelGrid);
        }
    }

    void getGaussianApproxBoxRadiuses(double sigma, std::vector<int>& boxRadius)
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
            boxRadius[k] = (k < m)?wlr:wur;
    }

    void applyRowBlur(uchar* source, uchar* target, const cv::Size& size, int lineRadius)
    {
        double invLineSize = 1. / (double)(2 * lineRadius + 1);
        int accumulator[3];
        int start[3];
        int end[3];

        for (int i = 0; i < size.height; i++)
        {
            // Average filter start middle and end indices
            int startId = 3 * i * size.width;
            int endId = 3 * (i * size.width + lineRadius);
            int targetId = startId;

            // Initialization 
            for (int c = 0; c < 3; c++)
            {
                start[c] = source[startId + c];
                end[c] = source[3 * ((i + 1) * size.width - 1) + c];
                accumulator[c] = start[c] * (lineRadius + 1);
            }

            for (int j = 0; j < lineRadius; j++)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[startId + 3 * j + c];
                }
            }

            // Average filtering
            for (int j = 0; j <= lineRadius; j++, targetId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[endId + c] - start[c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }

            for (int j = lineRadius + 1; j < size.width - lineRadius; j++, targetId += 3, startId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[endId + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }

            for (int j = size.width - lineRadius; j < size.width; j++, targetId += 3, startId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += end[c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }
        }
    }

    void applyColBlur(uchar* source, uchar* target, const cv::Size& size, int lineRadius)
    {
        double invLineSize = 1. / (double)(2 * lineRadius + 1);
        std::vector<int> accumulator(3 * size.width);
        std::vector<int> start(3 * size.width);
        std::vector<int> end(3 * size.width);

        // Average filter start middle and end indices
        int startId = 0;
        int endId = 3 * (lineRadius * size.width);
        int targetId = 0;

        // Initialization 
        for (int j = 0; j < size.width; j++)
        {
            for (int c = 0; c < 3; c++)
            {
                start[3 * j + c] = source[3 * j + c];
                end[3 * j + c] = source[3 * ((size.height - 1) * size.width + j) + c];
                accumulator[3 * j + c] = start[3 * j + c] * (lineRadius + 1);
            }
        }

        for (int i = 0; i < lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[3 * (i * size.width + j) + c];
                }
            }
        }

        // Average filtering
        for (int i = 0; i <= lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[endId + c] - start[3 * j + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }

        for (int i = lineRadius + 1; i < size.height - lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, startId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[endId + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }

        for (int i = size.height - lineRadius; i < size.height; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, startId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += end[3 * j + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }
    }

    void applyBoxBlur(uchar* image, uchar* temp, const cv::Size& size, int boxRadius)
    {
        applyRowBlur(image, temp, size, boxRadius);
        applyColBlur(temp, image, size, boxRadius);
    }
}


/*
    Utils methods implementation 
*/

void Utils::computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Point& cropFirstPixel, const cv::Size& cropSize)
{
    cv::Mat croppedImage(cropSize, CV_8UC3, cv::Scalar(0, 0, 0));
    if (croppedImage.size() != source.size())
    {
        int step = source.size().width;
        for (int i = 0; i < cropSize.height; i++)
        {
            for (int j = 0; j < cropSize.width; j++)
            {
                int imageId = getDataIndex(cropFirstPixel.y + i, cropFirstPixel.x + j, step);
                int imageToProcessId = getDataIndex(i, j, cropSize.width);
                for (int c = 0; c < 3; c++)
                    croppedImage.data[imageToProcessId + c] = source.data[imageId + c];
            }
        }
    }
    else
    {
        croppedImage = source;
    }

    target = cv::Mat(targetSize, CV_8UC3, cv::Scalar(0, 0, 0));
    double wScaleInv = (double)cropSize.width / (double)targetSize.width;
    double hScaleInv = (double)cropSize.height / (double)targetSize.height;
    double minScaleInv = std::min(wScaleInv, hScaleInv);

    if (minScaleInv != 1.)
    {
        if (minScaleInv > 1.) //Use area interpolation for image downscaling
        {
            for (int i = 0; i < targetSize.height; i++)
            {
                for (int j = 0; j < targetSize.width; j++)
                {
                    int dataId = getDataIndex(i, j, targetSize.width);
                    computeAreaInterpolationPixel(&target.data[dataId], croppedImage.data, cropSize, i, j, minScaleInv);
                }
            }
        }
        else //Use bicubic interpolation for image upscaling
        {
            //Use blurring only for downscaling
            //double blurSigma = minScale / 3.;
            //applyGaussianBlur(croppedImage.data, cropSize, blurSigma, BLUR_NB_BOXES);

            for (int i = 0; i < targetSize.height; i++)
            {
                for (int j = 0; j < targetSize.width; j++)
                {
                    int dataId = getDataIndex(i, j, targetSize.width);
                    computeBicubicInterpolationPixel(&target.data[dataId], croppedImage.data, cropSize, i, j, minScaleInv);
                }
            }
        }
    }
    else
    {
        for (int k = 0; k < 3 * targetSize.width * targetSize.height; k++)
            target.data[k] = croppedImage.data[k];
    }
}

void Utils::applyGaussianBlur(uchar* image, const cv::Size& size, double sigma, int nbBoxes)
{
    uchar* temp = new uchar[3 * size.width * size.height];
    if (!temp)
        return;

    std::vector<int> boxRadius(nbBoxes);
    getGaussianApproxBoxRadiuses(sigma, boxRadius);

    if (boxRadius[boxRadius.size() - 1] <= std::min(size.width, size.height) / 2)
    {
        for (int k = 0; k < boxRadius.size(); k++)
            applyBoxBlur(image, temp, size, boxRadius[k]);
    }

    delete[] temp;
}

void Utils::computeImageBGRFeatures(const uchar* image, const cv::Size& size, const cv::Point& firstPos, int step, double* features, int featureDirSubdivision)
{
    int blockWidth = (int)ceil(size.width / (double)featureDirSubdivision);
    int blockHeight = (int)ceil(size.height / (double)featureDirSubdivision);

    for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++)
        features[k] = 0;

    for (int i = 0; i < size.height; i++)
    {
        for (int j = 0; j < size.width; j++)
        {
            int blockId = featureDirSubdivision * (i / blockHeight) + j / blockWidth;
            int imageId = getDataIndex(firstPos.y + i, firstPos.x + j, step);
            for (int c = 0; c < 3; c++)
                features[3 * blockId + c] += image[imageId + c];
        }
    }

    for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++)
    {
        int corrBlockHeight = (k < featureDirSubdivision * (featureDirSubdivision - 1) * 3)?blockHeight:size.height - (featureDirSubdivision - 1) * blockHeight;
        int corrBlockWidth = (((k / 4 + 1) % featureDirSubdivision) != 0)?blockWidth:size.width - (featureDirSubdivision - 1) * blockWidth;
        features[k] /= corrBlockWidth * corrBlockHeight;
    }
}

double Utils::BGRFeatureDistance(const double* vec1, const double* vec2, int size)
{
    //Use deltaE distance
    double sumDist = 0.;
    for (int i = 0; i < 3 * size; i += 3)
    {
        double dB = vec1[i] - vec2[i];
        double dG = vec1[i + 1] - vec2[i + 1];
        double dR = vec1[i + 2] - vec2[i + 2];
        double mR = (vec1[i + 2] + vec2[i + 2]) / 2.;
        double sqDist = (2. + mR / 256.) * dR * dR + 4 * dG * dG + (2. + (255. - mR) / 256.) * dB * dB;
        sumDist += sqrt(sqDist);
    }

    return sumDist;
}

void Utils::convertBGRtoHSV(double& hue, double& saturation, double& value, uchar blue, uchar green, uchar red)
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

void Utils::convertHSVtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double value)
{
    double C = value * saturation;
    double H = hue * 3. / M_PI;
    double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

    double B, G, R;
    if (H < 0. || 6. < H)
    {
        B = 0;
        G = 0;
        R = 0;
    }
    else if (H <= 1.)
    {
        B = 0.;
        G = X;
        R = C;
    }
    else if (H <= 2.)
    {
        B = 0.;
        G = C;
        R = X;
    }
    else if (H <= 3.)
    {
        B = X;
        G = C;
        R = 0.;
    }
    else if (H <= 4.)
    {
        B = C;
        G = X;
        R = 0.;
    }
    else if (H <= 5.)
    {
        B = C;
        G = 0.;
        R = X;
    }
    else if (H <= 6.)
    {
        B = X;
        G = 0.;
        R = C;
    }

    double m = value - C;

    blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
    red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
}

void Utils::convertBGRtoHSL(double& hue, double& saturation, double& lightness, uchar blue, uchar green, uchar red)
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

void Utils::convertHSLtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double lightness)
{
    double C = (1. - std::abs(2. * lightness - 1.)) * saturation;
    double H = hue * 3. / M_PI;
    double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

    double B, G, R;
    if (H < 0. || 6. < H)
    {
        B = 0;
        G = 0;
        R = 0;
    }
    else if (H <= 1.)
    {
        B = 0.;
        G = X;
        R = C;
    }
    else if (H <= 2.)
    {
        B = 0.;
        G = C;
        R = X;
    }
    else if (H <= 3.)
    {
        B = X;
        G = C;
        R = 0.;
    }
    else if (H <= 4.)
    {
        B = C;
        G = X;
        R = 0.;
    }
    else if (H <= 5.)
    {
        B = C;
        G = 0.;
        R = X;
    }
    else if (H <= 6.)
    {
        B = X;
        G = 0.;
        R = C;
    }

    double m = lightness - C / 2.;

    blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
    red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
}

void Utils::convertBGRtoHSI(double& hue, double& saturation, double& intensity, uchar blue, uchar green, uchar red)
{
    double B = blue / 255.;
    double G = green / 255.;
    double R = red / 255.;
    double i = B + G + R;
    intensity = i / 3;

    if (R == G && G == B)
    {
        hue = 0.;
        saturation = 0.;
    }
    else
    {
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

void Utils::convertHSItoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double intensity)
{
    double B, G, R;

    if (saturation == 0.)
        B = G = R = intensity;
    else
    {
        if ((hue >= 0.) && (hue < 2. * M_PI / 3.))
        {
            B = (1. - saturation) / 3.;
            R = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
            G = 1. - R - B;
        }
        else if ((hue >= 2. * M_PI / 3.) && (hue < 4. * M_PI / 3.))
        {
            hue = hue - 2. * M_PI / 3.;
            R = (1. - saturation) / 3.;
            G = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
            B = 1. - R - G;
        }
        else if ((hue >= 4. * M_PI / 3.) && (hue < 2. * M_PI))
        {
            hue = hue - 4. * M_PI / 3.;
            G = (1. - saturation) / 3.;
            B = (1. + saturation * cos(hue) / cos(M_PI / 3. - hue)) / 3.;
            R = 1. - B - G;
        }
        else
        {
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

void Utils::convertBGRtoLUV(double& L, double& u, double& v, uchar blue, uchar green, uchar red)
{
    //Using illuminant D65 as white reference

    if (blue == 0 && green == 0 && red == 0)
    {
        L = 0.;
        u = 0.;
        v = 0.;
        return;
    }

    double R = (double)red / 255.;
    double G = (double)green / 255.;
    double B = (double)blue / 255.;

    double X = (0.49 * R + 0.31 * G + 0.2 * B) / 0.17697;
    double Y = (0.17697 * R + 0.8124 * G + 0.01063 * B) / 0.17697;
    double Z = (0.01 * G + 0.99 * B) / 0.17697;

    double Y_norm = Y / 100.;
    double div = X + 15. * Y + 3. * Z;
    double u_p = 4. * X / div;
    double v_p = 9. * Y / div;

    if (Y_norm > 0.008856)
        L = 116. * cubeRoot(Y_norm) - 16.;
    else
        L = Y_norm * 903.296296;

    u = 13. * L * (u_p - 0.197840);
    v = 13. * L * (v_p - 0.468336);

    L = clip<double>(L, 0., 100.);
    u = clip<double>(u, -100., 100.);
    v = clip<double>(v, -100., 100.);
}

void Utils::convertLUVtoBGR(uchar& blue, uchar& green, uchar& red, double L, double u, double v)
{
    //Using illuminant D65 as white reference

    if (L == 0. && u == 0. && v == 0.)
    {
        blue = 0;
        green = 0;
        red = 0;
        return;
    }

    double u_p = u / (13. * L) + 0.197840;
    double v_p = v / (13. * L) + 0.468336;

    double Y;
    if (L > 8.)
        Y = 100. * (L + 16.) * (L + 16.) * (L + 16.) / 1560896.;
    else
        Y = 100. * L * 0.001107;

    double X = Y * 9. * u_p / (4. * v_p);
    double Z = Y * (12. - 3. * u_p - 20. * v_p) / (4. * v_p);

    red = (uchar)clip<int>((int)round((0.41847 * X - 0.15866 * Y - 0.082835 * Z) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((-0.091169 * X + 0.25243 * Y + 0.015708 * Z) * 255.), 0, 255);
    blue = (uchar)clip<int>((int)round((0.0009209 * X - 0.0025498 * Y + 0.1786 * Z) * 255.), 0, 255);
}

void Utils::convertBGRtoLAB(double& L, double& a, double& b, uchar blue, uchar green, uchar red)
{
    //Using illuminant D65 as white reference

    if (blue == 0 && green == 0 && red == 0)
    {
        L = 0.;
        a = 0.;
        b = 0.;
        return;
    }

    double R = (double)red / 255.;
    double G = (double)green / 255.;
    double B = (double)blue / 255.;

    double X_norm = (0.49 * R + 0.31 * G + 0.2 * B) / 16.820804;
    double Y_norm = (0.17697 * R + 0.8124 * G + 0.01063 * B) / 17.697;
    double Z_norm = (0.01 * G + 0.99 * B) / 19.269201;

    double X_var, Y_var, Z_var;

    if (X_norm > 0.008856)
        X_var = cubeRoot(X_norm);
    else
        X_var = X_norm * 7.787037 + 0.137931;

    if (Y_norm > 0.008856)
        Y_var = cubeRoot(Y_norm);
    else
        Y_var = Y_norm * 7.787037 + 0.137931;

    if (Z_norm > 0.008856)
        Z_var = cubeRoot(Z_norm);
    else
        Z_var = Z_norm * 7.787037 + 0.137931;

    L = 116. * Y_var - 16.;
    a = 500 * (X_var - Y_var);
    b = 200 * (Y_var - Z_var);

    L = clip<double>(L, 0., 100.);
    a = clip<double>(a, -100., 100.);
    b = clip<double>(b, -100., 100.);
}

void Utils::convertLABtoBGR(uchar& blue, uchar& green, uchar& red, double L, double a, double b)
{
    //Using illuminant D65 as white reference

    if (L == 0. && a == 0. && b == 0.)
    {
        blue = 0;
        green = 0;
        red = 0;
        return;
    }

    double Y_var = (L + 16.) / 116.;
    double X_var = Y_var + a / 500.;
    double Z_var = Y_var - b / 200.;

    double X, Y, Z;

    if (X_var > 0.206896)
        X = 95.0489 * X_var * X_var * X_var;
    else
        X = 12.206042 * X_var - 1.683594;

    if (Y_var > 0.206896)
        Y = 100. * Y_var * Y_var * Y_var;
    else
        Y = 12.841855 * Y_var - 1.771290;

    if (Z_var > 0.206896)
        Z = 108.884 * Z_var * Z_var * Z_var;
    else
        Z = 13.982725 * Z_var - 1.928652;

    red = (uchar)clip<int>((int)round((0.41847 * X - 0.15866 * Y - 0.082835 * Z) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((-0.091169 * X + 0.25243 * Y + 0.015708 * Z) * 255.), 0, 255);
    blue = (uchar)clip<int>((int)round((0.0009209 * X - 0.0025498 * Y + 0.1786 * Z) * 255.), 0, 255);
}



