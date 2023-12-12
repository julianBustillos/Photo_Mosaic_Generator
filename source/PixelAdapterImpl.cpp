#include "PixelAdapterImpl.h"
#include "Utils.h"
#include "variables.h"
#include "CustomException.h"


void PixelAdapterImpl::compute()
{
    _tileCorrection.resize(_subdivisions * _subdivisions);

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            computeAdapterData(_tileCorrection[mosaicId], _photo.getData(), _photo.getFirstPixel(i, j, true), _photo.getTileSize(), _photo.getStep());
        }
    }
}

void PixelAdapterImpl::applyCorrection(cv::Mat& tile, int mosaicId) const
{
    if (_tileCorrection.empty())
        throw CustomException("PixelAdapterImpl::compute() has not generated data !", CustomException::Level::ERROR);

    uchar* data = tile.data;
    cv::Size size = tile.size();
    AdapterData originalTile;
    computeAdapterData(originalTile, data, cv::Point(0, 0), size, size.width);

    int tileDataSize = 3 * size.width * size.height;

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

    for (int k = 0; k < tileDataSize; k += 3)
    {
        uchar originalBlue = data[k];
        uchar originalGreen = data[k + 1];
        uchar originalRed = data[k + 2];

        uchar matchingBlue = BGR_correction_function[0][originalBlue];
        uchar matchingGreen = BGR_correction_function[1][originalGreen];
        uchar matchingRed = BGR_correction_function[2][originalRed];

        data[k] = (uchar)(HISTOGRAM_CORRECTION_BLENDING * (double)matchingBlue + (1. - HISTOGRAM_CORRECTION_BLENDING) * (double)originalBlue);
        data[k + 1] = (uchar)(HISTOGRAM_CORRECTION_BLENDING * (double)matchingGreen + (1. - HISTOGRAM_CORRECTION_BLENDING) * (double)originalGreen);
        data[k + 2] = (uchar)(HISTOGRAM_CORRECTION_BLENDING * (double)matchingRed + (1. - HISTOGRAM_CORRECTION_BLENDING) * (double)originalRed);
    }
}

void PixelAdapterImpl::computeAdapterData(AdapterData& adapterData, const uchar* data, const cv::Point& firstPixel, const cv::Size& size, int step) const
{
    uchar blue, green, red;

    //Compute cumulative distribution function for BGR colors
    for (int c = 0; c < 3; c++)
    {
        for (int k = 0; k < 256; k++)
        {
            adapterData._BGR_cdf[c][k] = 0.;
        }
    }

    for (int i = 0; i < size.height; i++)
    {
        for (int j = 0; j < size.width; j++)
        {
            int id = Utils::getDataIndex(firstPixel.y + i, firstPixel.x + j, step);
            blue = data[id];
            green = data[id + 1];
            red = data[id + 2];

            adapterData._BGR_cdf[0][blue] += 1.;
            adapterData._BGR_cdf[1][green] += 1.;
            adapterData._BGR_cdf[2][red] += 1.;
        }
    }

    int nbPixel = size.width * size.height;
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
