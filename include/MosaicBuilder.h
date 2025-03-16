#pragma once

#include "Photo.h"
#include "PixelAdapter.h"
#include "Tiles.h"
#include "MatchSolver.h"
#include <vector>
#include <opencv2/opencv.hpp>



class MosaicBuilder
{
public:
    MosaicBuilder(int subdivisions, double blending, double blendingMin, double blendingMax);
    ~MosaicBuilder();
    void build(const Photo& photo, const PixelAdapter& pixelAdapter, const Tiles& tiles, const MatchSolver& matchSolver);

private:
    void copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const PixelAdapter& pixelAdapter, double blending, int mosaicId, const cv::Rect& box);
    void exportMosaic(const std::string& path, double _blending, cv::Mat mosaic);

private:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
    const double _blending;
    const double _blendingMin;
    const double _blendingMax;
};

