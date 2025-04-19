#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
#include "MathUtils.h"
#include <numbers>


ColorEnhancer::ColorEnhancer() :
    _w1Distance(-1)
{
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::computeData(const Photo& photo, const cv::Mat& tile, int mosaicId)
{
    ProbaUtils::CDF optimalCDF, targetCDF;
    ProbaUtils::imageCDF(optimalCDF, photo.getImage(), photo.getTileBox(mosaicId, true));
    ProbaUtils::imageCDF(targetCDF, tile);

    _w1Distance = ProbaUtils::w1Distance(optimalCDF, targetCDF);
    if (_w1Distance > W1DistTarget)
    {
        ProbaUtils::GMMCDFComponents components;
        bool found = true;
        for (int c = 0; c < 3 && found; c++)
        {
            //DEBUG
            std::vector<int> data;
            data.reserve(tile.size().width * tile.size().height);
            for (int d = 0; d < tile.size().width * tile.size().height; d++)
            {
                data.push_back(tile.data[3 * d + c]);
            }

            const int width = photo.getImage().size().width;
            const int step = 3 * (width - photo.getTileBox(mosaicId, true).width);
            int p = 3 * (photo.getTileBox(mosaicId, true).y * width + photo.getTileBox(mosaicId, true).x) + c;
            for (int i = 0; i < photo.getTileBox(mosaicId, true).height; i++, p += step)
            {
                for (int j = 0; j < photo.getTileBox(mosaicId, true).width; j++, p+= 3)
                {
                    data.push_back(photo.getImage().data[p]);
                }
            }
            //DEBUG 
            
            //TODO: better data computation (replace with CDF ?)
            found = GaussianMixtureModel::findOptimalComponents(components[c], data, CompoMaxNb, MaxIter, ConvergenceTol, true);
        }

        if (found)
        {
            found = findOptimalDistanceCDF(optimalCDF, components, targetCDF);
            if (found)
                _w1Distance = ProbaUtils::w1Distance(optimalCDF, targetCDF);
        }
    }

    for (int c = 0; c < 3; c++)
    {
        int optimalColor = 0;
        for (int k = 0; k < 256; k++)
        {
            double probability = targetCDF[c][k];
            while (probability > optimalCDF[c][optimalColor])
                optimalColor++;
            _colorMapping[c][k] = optimalColor;
        }
    }
}

uchar ColorEnhancer::apply(uchar color, int channel, double blending) const
{
    uchar optimalColor = _colorMapping[channel][color];
    double corrBlending = std::min(blending * W1DistTarget / _w1Distance, 1.);
    return (uchar)(corrBlending * (double)optimalColor + (1. - corrBlending) * (double)color);
}

bool ColorEnhancer::findOptimalDistanceCDF(ProbaUtils::CDF& optimalCDF, const ProbaUtils::GMMCDFComponents& components, const ProbaUtils::CDF& targetCDF) const
{
    double startConstr[3], endConstr[3];
    double varMean = 0.;
    for (int c = 0; c < 3; c++)
    {
        startConstr[c] = targetCDF[c][0];
        endConstr[c] = targetCDF[c][255] - targetCDF[c][254];

        for (int k = 0; k < components[c].size(); k++)
            varMean += components[c][k]._variance * components[c][k]._weight;
    }
    varMean /= 3.;
    double stdDevMean = sqrt(varMean);

    //Find interval [xMin, xMax] containing narest distance to target distance
    auto distance = [&](double x)
        {
            ProbaUtils::CDF gmmCDF;
            ProbaUtils::gmmCDF(gmmCDF, components, x, startConstr, endConstr);
            return abs(ProbaUtils::w1Distance(gmmCDF, targetCDF) - W1DistTarget);
        };

    const int nbSteps = (int)((StdDevMax - stdDevMean) / StdDevIncr);
    bool foundInterval = false; 
    double xMin = -1, xMid = -1, xMax = -1;
    double dist_xMin = MathUtils::DoubleMax;
    double dist_xMid = MathUtils::DoubleMax;
    double dist_xMax = MathUtils::DoubleMax;

    for (int step = 0; step < nbSteps; step++)
    {
        xMax = stdDevMean + step * StdDevIncr;
        xMax =xMax * xMax / varMean;
        dist_xMax = distance(xMax);

        if (dist_xMax > dist_xMid)
        {
            foundInterval = true;
            break;
        }

        xMin = xMid;
        xMid = xMax;
        dist_xMin = dist_xMid;
        dist_xMid = dist_xMax;
    }

    if (!foundInterval)
        return false;

    if (xMin < 0)
        xMin = xMid;

    //Golden section search algorithm
    while ((xMax - xMin) > ConvergenceTol)
    {
        double x0 = xMax - (xMax - xMin) / std::numbers::phi;
        double x1 = xMin + (xMax - xMin) / std::numbers::phi;
        if (distance(x0) < distance(x1))
            xMax = x1;
        else
            xMin = x0;
    }

    double xOpt = (xMax + xMin) * 0.5;
    ProbaUtils::gmmCDF(optimalCDF, components, xOpt, startConstr, endConstr);

    return true;
}

