#pragma once

#include "photo.h"
#include "tiles.h"
#include <opencv2/opencv.hpp>



class MosaicBuilder {
public:
    MosaicBuilder (const Photo &photo, const Tiles &tiles, int subdivisions, int *matchingTiles);
    ~MosaicBuilder() {};

private:
    void copyTileOnMosaic(uchar *mosaicData, const std::string &tilePath, const cv::Point firstPixelPos, const int step);
    void exportMosaic(const std::string &path, cv::Mat mosaic);
    void printInfo() const;
};

