#include "PixelAdapter.h"
#include "MathUtils.h"
#include "CustomException.h"
#include "Log.h"
#include "Console.h"


PixelAdapter::PixelAdapter(int subdivisions) :
    _subdivisions(subdivisions)
{
}

PixelAdapter::~PixelAdapter()
{
}

void PixelAdapter::compute(const Photo& photo)
{
    Console::Out::get(Console::DEFAULT) << "Computing image correction data...";
    _tileCorrection.resize(_subdivisions * _subdivisions);

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            computeAdapterData(_tileCorrection[mosaicId], photo.getImage(), photo.getTileBox(i, j, true));
        }
    }
    Log::Logger::get().log(Log::TRACE) << "Adapter data computed.";
}

void PixelAdapter::applyCorrection(cv::Mat& tile, double blending, int mosaicId) const
{
    cv::Rect box(0, 0, tile.size().width, tile.size().height);
    AdapterData originalTile;
    computeAdapterData(originalTile, tile, box);

    int tileDataSize = 3 * box.width * box.height;

    int BGR_correction_function[3][256];
    for (int c = 0; c < 3; c++)
    {
        int matchingValue = 0;
        for (int k = 0; k < 256; k++)
        {
            double probability = originalTile._BGR_cdf[c][k];
            while (probability > _tileCorrection[mosaicId]._BGR_cdf[c][matchingValue])
                matchingValue++;
            BGR_correction_function[c][k] = matchingValue;
        }
    }

    uchar* data = tile.data;

    for (int k = 0; k < tileDataSize; k += 3)
    {
        uchar originalBlue = data[k];
        uchar originalGreen = data[k + 1];
        uchar originalRed = data[k + 2];

        uchar matchingBlue = BGR_correction_function[0][originalBlue];
        uchar matchingGreen = BGR_correction_function[1][originalGreen];
        uchar matchingRed = BGR_correction_function[2][originalRed];

        data[k] = (uchar)(blending * (double)matchingBlue + (1. - blending) * (double)originalBlue);
        data[k + 1] = (uchar)(blending * (double)matchingGreen + (1. - blending) * (double)originalGreen);
        data[k + 2] = (uchar)(blending * (double)matchingRed + (1. - blending) * (double)originalRed);
    }
}

void PixelAdapter::computeAdapterData(AdapterData& adapterData, const cv::Mat& image, const cv::Rect& box) const
{
    //Compute cumulative distribution function for BGR colors
    for (int c = 0; c < 3; c++)
    {
        for (int k = 0; k < 256; k++)
        {
            adapterData._BGR_cdf[c][k] = 0.;
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
                adapterData._BGR_cdf[c][image.data[p]] += 1.;
            }
        }
    }

    int nbPixel = box.width * box.height;
    for (int c = 0; c < 3; c++)
    {
        for (int k = 1; k < 256; k++)
        {
            adapterData._BGR_cdf[c][k] += adapterData._BGR_cdf[c][k - 1];
            adapterData._BGR_cdf[c][k - 1] /= nbPixel;
        }
        adapterData._BGR_cdf[c][255] /= nbPixel;
    }
}
