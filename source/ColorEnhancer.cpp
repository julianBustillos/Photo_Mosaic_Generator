#include "ColorEnhancer.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"


ColorEnhancer::ColorEnhancer(std::tuple<int, int> grid) :
    _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid))
{
}

ColorEnhancer::~ColorEnhancer()
{
}

void ColorEnhancer::computeData(const Photo& photo)
{
    Console::Out::get(Console::DEFAULT) << "Computing image correction data...";
    _tileData.resize(_gridWidth * _gridHeight);

    for (int i = 0; i < _gridHeight; i++)
    {
        for (int j = 0; j < _gridWidth; j++)
        {
            int mosaicId = i * _gridWidth + j;
            computeTileData(_tileData[mosaicId], photo.getImage(), photo.getTileBox(i, j, true));
        }
    }
    Log::Logger::get().log(Log::TRACE) << "Adapter data computed.";
}

void ColorEnhancer::apply(cv::Mat& tile, double blending, int mosaicId) const
{
    cv::Rect box(0, 0, tile.size().width, tile.size().height);
    EnhancerData originalTileData;
    computeTileData(originalTileData, tile, box);

    int tileDataSize = 3 * box.width * box.height;

    int colorCorrection[3][256];
    for (int c = 0; c < 3; c++)
    {
        int matchingValue = 0;
        for (int k = 0; k < 256; k++)
        {
            double probability = originalTileData._colorCDF[c][k];
            while (probability > _tileData[mosaicId]._colorCDF[c][matchingValue])
                matchingValue++;
            colorCorrection[c][k] = matchingValue;
        }
    }

    uchar* data = tile.data;
    for (int k = 0; k < tileDataSize; k += 3)
    {
        for (int c = 0; c < 3; c++)
        {
            uchar originalColor = data[k + c];
            uchar correctedColor = colorCorrection[c][originalColor];
            data[k + c] = (uchar)(blending * (double)correctedColor + (1. - blending) * (double)originalColor);
        }
    }
}

void ColorEnhancer::computeTileData(EnhancerData& data, const cv::Mat& image, const cv::Rect& box) const
{
    //Compute cumulative distribution function for BGR colors
    for (int c = 0; c < 3; c++)
    {
        for (int k = 0; k < 256; k++)
        {
            data._colorCDF[c][k] = 0.;
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
                data._colorCDF[c][image.data[p]] += 1.;
            }
        }
    }

    int nbPixel = box.width * box.height;
    for (int c = 0; c < 3; c++)
    {
        for (int k = 1; k < 256; k++)
        {
            data._colorCDF[c][k] += data._colorCDF[c][k - 1];
            data._colorCDF[c][k - 1] /= nbPixel;
        }
        data._colorCDF[c][255] /= nbPixel;
    }
}
