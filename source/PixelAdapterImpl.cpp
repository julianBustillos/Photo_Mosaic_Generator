#include "PixelAdapterImpl.h"
#include "MathUtils.h"
#include "CustomException.h"


void PixelAdapterImpl::compute()
{
    _tileCorrection.resize(_subdivisions * _subdivisions);

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            computeAdapterData(_tileCorrection[mosaicId], _photo->getImage(), _photo->getTileBox(i, j, true));
        }
    }
}

void PixelAdapterImpl::applyCorrection(cv::Mat& tile, int mosaicId) const
{
    if (_tileCorrection.empty())
        throw CustomException("PixelAdapterImpl::compute() has not generated data !", CustomException::Level::ERROR);

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

        data[k] = (uchar)(HistogramCorrectionBlending * (double)matchingBlue + (1. - HistogramCorrectionBlending) * (double)originalBlue);
        data[k + 1] = (uchar)(HistogramCorrectionBlending * (double)matchingGreen + (1. - HistogramCorrectionBlending) * (double)originalGreen);
        data[k + 2] = (uchar)(HistogramCorrectionBlending * (double)matchingRed + (1. - HistogramCorrectionBlending) * (double)originalRed);
    }
}

void PixelAdapterImpl::computeAdapterData(AdapterData& adapterData, const cv::Mat& image, const cv::Rect& box) const
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

    for (int i = 0; i < box.height; i++)
    {
        for (int j = 0; j < box.width; j++)
        {
            blue = image.ptr(box.y + i, box.x + j)[0];
            green = image.ptr(box.y + i, box.x + j)[1];
            red = image.ptr(box.y + i, box.x + j)[2];

            adapterData._BGR_cdf[0][blue] += 1.;
            adapterData._BGR_cdf[1][green] += 1.;
            adapterData._BGR_cdf[2][red] += 1.;
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
