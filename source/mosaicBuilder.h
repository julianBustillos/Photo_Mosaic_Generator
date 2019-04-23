#pragma once

#include "photo.h"
#include "pixelAdapter.h"
#include "tiles.h"
#include <vector>
#include <opencv2/opencv.hpp>



class MosaicBuilder {
public:
    MosaicBuilder (const Photo &photo, const PixelAdapter &pixelAdapter, const Tiles &tiles, int subdivisions, const std::vector<int> &matchingTiles);
    ~MosaicBuilder() {};

private:
    void copyTileOnMosaic(uchar *mosaicData, const std::string &tilePath, const PixelAdapter &pixelAdapter, int mosaicId, const cv::Point firstPixelPos, int step);
    void exportMosaic(const std::string &path, cv::Mat mosaic);
    void printInfo() const;
};

