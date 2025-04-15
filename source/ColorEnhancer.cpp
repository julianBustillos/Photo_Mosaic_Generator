#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"


ColorEnhancer::ColorEnhancer()
{
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::computeData(const Photo& photo, const cv::Mat& tile, int mosaicId)
{
    ProbaUtils::CDF _referenceCDF, _targetCDF;
    cv::Rect box(0, 0, tile.size().width, tile.size().height);

    computeTileCDF(_referenceCDF, photo.getImage(), photo.getTileBox(mosaicId, true));
    computeTileCDF(_targetCDF, tile, box);

    //TODO: compute CDF
    //TODO: check WDistance
    //TODO: find optimal GMM
    //TODO: find optimal target dist
    //TODO: compute correction mapping
    //TODO: compute real blending value
    //TODO: apply blending

    for (int c = 0; c < 3; c++)
    {
        int optimalColor = 0;
        for (int k = 0; k < 256; k++)
        {
            double probability = _targetCDF[c][k];
            while (probability > _referenceCDF[c][optimalColor])
                optimalColor++;
            _colorMapping[c][k] = optimalColor;
        }
    }
}

uchar ColorEnhancer::apply(uchar color, int channel, double blending) const
{
    uchar optimalColor = _colorMapping[channel][color];
    return (uchar)(blending * (double)optimalColor + (1. - blending) * (double)color);
}

void ColorEnhancer::computeTileCDF(ProbaUtils::CDF& cdf, const cv::Mat& image, const cv::Rect& box) const
{
    //Compute cumulative distribution function for BGR colors
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
