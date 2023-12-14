#pragma once

#include "Photo.h"
#include "IPixelAdapter.h"
#include "ITiles.h"
#include "IMatchSolver.h"
#include <vector>
#include <opencv2/opencv.hpp>



class MosaicBuilder
{
public:
    MosaicBuilder(const Photo& photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};
    void Build(const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver);
    ~MosaicBuilder() {};

private:
    void copyTileOnMosaic(uchar* mosaicData, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Point firstPixel, int step);
    void exportMosaic(const std::string& path, cv::Mat mosaic);
    void printInfo() const;

private:
    const Photo& _photo;
    const int _subdivisions;
};

