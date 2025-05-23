#pragma once

#include "Photo.h"
#include "ColorEnhancer.h"
#include "Tiles.h"
#include "MatchSolver.h"
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>



class MosaicBuilder
{
private:
    static constexpr int MosaicParam[2] = {cv::IMWRITE_JPEG_QUALITY, 100};

public:
    MosaicBuilder(std::tuple<int, int> grid, std::tuple<double, double, double> blending);
    ~MosaicBuilder();
    void build(const Photo& photo, const Tiles& tiles, const MatchSolver& matchSolver);

private:
    void copyTileOnMosaic(cv::Mat& mosaic, const cv::Mat& tile, const ColorEnhancer& colorEnhancer, double blending, int mosaicId, const cv::Rect& box);
    void exportMosaic(const std::string& path, double _blending, cv::Mat mosaic);

private:
    std::shared_ptr<const Photo> _photo;
    const int _gridWidth;
    const int _gridHeight;
    const double _blendingStep;
    const double _blendingMin;
    const double _blendingMax;
};

