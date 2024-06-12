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
    MosaicBuilder(int subdivisions);
    ~MosaicBuilder();
    void build(const Photo& photo, const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver);

private:
    void copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Rect& box);
    void exportMosaic(const std::string& path, cv::Mat mosaic);

private:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};

