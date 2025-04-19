#include "ProbaUtils.h"
#include "MathUtils.h"
#include <numbers>


ProbaUtils::CDF::CDF()
{
    for (int c = 0; c < 758; c++)
        _data[c] = 0;
}

double* ProbaUtils::CDF::operator[](int color)
{
    return &_data[256 * color];
}

const double* ProbaUtils::CDF::operator[](int color) const
{
    return &_data[256 * color];
}

void ProbaUtils::imageCDF(CDF& cdf, const cv::Mat& image)
{
    cv::Rect box(0, 0, image.size().width, image.size().height);
    imageCDF(cdf, image, box);
}

void ProbaUtils::imageCDF(CDF& cdf, const cv::Mat& image, const cv::Rect& box)
{
    for (int c = 0; c < 3; c++)
    {
        for (int k = 0; k < 256; k++)
        {
            cdf[c][k] = 0.;
        }
    }

    const int width = image.size().width;
    const int step = 3 * (width - box.width);
    int p = 3 * (box.y * width + box.x);
    for (int i = 0; i < box.height; i++, p += step)
    {
        for (int j = 0; j < box.width; j++)
        {
            for (int c = 0; c < 3; c++, p++)
            {
                cdf[c][image.data[p]] += 1.;
            }
        }
    }

    int nbPixel = box.width * box.height;
    for (int c = 0; c < 3; c++)
    {
        for (int k = 1; k < 256; k++)
        {
            cdf[c][k] += cdf[c][k - 1];
            cdf[c][k - 1] /= nbPixel;
        }
        cdf[c][255] /= nbPixel;
    }
}

double ProbaUtils::evalGaussianCDF(double x, const GaussianComponent& component, double varScale)
{
    return 0.5 * (1 + erf((x - component._mean) / sqrt(2. * component._variance * varScale))) * component._weight;
}

double ProbaUtils::evalGmmCDF(double x, const GMMComponents& components, double varScale)
{
    double gmmCDF = 0;
    for (int c = 0; c < components.size(); c++)
    {
        gmmCDF += evalGaussianCDF(x, components[c], varScale);
    }
    return gmmCDF;
}

void ProbaUtils::gmmCDF(CDF& cdf, const GMMCDFComponents& components, double varScale, double startConstr[3], double endConst[3])
{
    for (int c = 0; c < 3; c++)
    {
        cdf[c][255] = evalGmmCDF(255, components[c], varScale);
        for (int i = 0; i < 255; i++)
            cdf[c][i] = evalGmmCDF(i, components[c], varScale) / cdf[c][255];
        cdf[c][255] = 1.;

        double min = 0.;
        double max = 1.;

        if (startConstr[c] < cdf[c][0])
            min = cdf[c][0] - startConstr[c];

        if (endConst[c] > (cdf[c][255] - cdf[c][254]))
            max = 1. - (endConst[c] - (cdf[c][255] - cdf[c][254]));

        double scale = max - min;
        if (scale < 1.)
        {
            for (int i = 0; i < 256; i++)
            {
                cdf[c][i] = (cdf[c][i] - min) / scale;
                if (cdf[c][i] > 1.)
                    cdf[c][i] = 1.;
            }
        }
    }
}

double ProbaUtils::w1Distance(const CDF& cdf0, const CDF& cdf1)
{
    double distance = 0;

    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 256; i++)
            distance += abs(cdf1[c][i] - cdf0[c][i]);
        
    return distance;
}
