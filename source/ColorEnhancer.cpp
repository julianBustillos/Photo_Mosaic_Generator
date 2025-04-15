#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"


ColorEnhancer::ColorEnhancer(int gridWidth, int gridHeight) :
    _gridWidth(gridWidth), _gridHeight(gridHeight)
{
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::computeData(const Photo& photo)
{
    Console::Out::get(Console::DEFAULT) << "Computing image color enhancement data...";
    _tileCDF.resize(_gridWidth * _gridHeight);

    for (int mosaicId = 0; mosaicId < _gridWidth * _gridHeight; mosaicId++)
    {
        computeTileCDF(_tileCDF[mosaicId], photo.getImage(), photo.getTileBox(mosaicId, true));
    }
    Log::Logger::get().log(Log::TRACE) << "Enhancement data computed.";
}

void ColorEnhancer::apply(cv::Mat& tile, double blending, int mosaicId) const
{
    //TODO: compute CDF
    //TODO: check WDistance
    //TODO: find optimal GMM
    //TODO: find optimal target dist
    //TODO: compute correction mapping
    //TODO: compute real blending value
    //TODO: apply blending

    cv::Rect box(0, 0, tile.size().width, tile.size().height);
    ProbaUtils::CDF originalTileData;
    computeTileCDF(originalTileData, tile, box);

    int tileDataSize = 3 * box.width * box.height;

    int colorMapping[3][256];
    for (int c = 0; c < 3; c++)
    {
        int optimalColor = 0;
        for (int k = 0; k < 256; k++)
        {
            double probability = originalTileData[c][k];
            while (probability > _tileCDF[mosaicId][c][optimalColor])
                optimalColor++;
            colorMapping[c][k] = optimalColor;
        }
    }

    uchar* data = tile.data;
    for (int k = 0; k < tileDataSize; k += 3)
    {
        for (int c = 0; c < 3; c++)
        {
            uchar originalColor = data[k + c];
            uchar optimalColor = colorMapping[c][originalColor];
            data[k + c] = (uchar)(blending * (double)optimalColor + (1. - blending) * (double)originalColor);
        }
    }
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
