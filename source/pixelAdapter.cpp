#include "pixelAdapter.h"
#include "mathTools.h"


PixelAdapter::PixelAdapter(Photo photo, int subdivisions)
{
    _tileCorrection.resize(subdivisions * subdivisions);

    for (int i = 0; i < subdivisions; i++) {
        for (int j = 0; j < subdivisions; j++) {
            int mosaicId = i * subdivisions + j;
            computeAdapterData(_tileCorrection[mosaicId], photo.getData(), photo.getFirstPixel(i, j, true), photo.getTileSize(), photo.getStep());
        }
    }
}

void PixelAdapter::applyCorrection(cv::Mat & tile, int mosaicId) const
{
    uchar *data = tile.data;
    cv::Size size = tile.size();
    AdapterData originalTile;
    computeAdapterData(originalTile, data, cv::Point(0, 0), size, size.width);

    int tileDataSize = 3 * size.width * size.height;

    int BGR_correction_function[3][256];
    for (int c = 0; c < 3; c++) {
        int matchingValue = 0;
        for (int k = 0; k < 256; k++) {
            double probability = originalTile._BGR_cdf[c][k];
            while (probability > _tileCorrection[mosaicId]._BGR_cdf[c][matchingValue])
                matchingValue++;
            BGR_correction_function[c][k] = matchingValue;
        }
    }

    for (int k = 0; k < tileDataSize; k += 3) {
        uchar blue = data[k];
        uchar green = data[k + 1];
        uchar red = data[k + 2];

        data[k] = BGR_correction_function[0][blue];
        data[k + 1] = BGR_correction_function[1][green];
        data[k + 2] = BGR_correction_function[2][red];
    }
}

void PixelAdapter::computeAdapterData(AdapterData &adapterData, const uchar *data, const cv::Point &firstPixel, const cv::Size &size, int step) const
{
    uchar blue, green, red;

    //Compute cumulative distribution function for BGR colors
    for (int c = 0; c < 3; c++) {
        for (int k = 0; k < 256; k++) {
            adapterData._BGR_cdf[c][k] = 0.;
        }
    }

    for (int i = 0; i < size.height; i++) {
        for (int j = 0; j < size.width; j++) {
            int id = MathTools::getDataIndex(firstPixel.y + i, firstPixel.x + j, step);
            blue = data[id];
            green = data[id + 1];
            red = data[id + 2];

            adapterData._BGR_cdf[0][blue] += 1.;
            adapterData._BGR_cdf[1][green] += 1.;
            adapterData._BGR_cdf[2][red] += 1.;
        }
    }

    int nbPixel = size.width * size.height;
    for (int c = 0; c < 3; c++) {
        for (int k = 1; k < 256; k++) {
            adapterData._BGR_cdf[c][k] += adapterData._BGR_cdf[c][k - 1];
            adapterData._BGR_cdf[c][k - 1] /= nbPixel;
        }
        adapterData._BGR_cdf[c][255] /= nbPixel;
    }
}