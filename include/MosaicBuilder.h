#pragma once

#include "Photo.h"
#include "Tiles.h"
#include "MatchSolver.h"
#include "ProbaUtils.h"
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>



class MosaicBuilder
{
private:
    static constexpr int MaxNbCompo = 10;
    static constexpr int NbInit = 20;
    static constexpr int MaxIter = 1000;
    static constexpr double ConvergenceTol = 1e-3;
    static constexpr double CovarianceReg = 1e-6;
    static constexpr int ColorEnhancerNbSamples = 1e6;
    static constexpr int MosaicParam[2] = {cv::IMWRITE_JPEG_QUALITY, 100};

public:
    MosaicBuilder(std::tuple<int, int> grid, std::tuple<double, double, double> blending);
    ~MosaicBuilder();
    void build(const Photo& photo, const Tiles& tiles, const MatchSolver& matchSolver);

private:
    void copyTileOnMosaic(cv::Mat& mosaic, const cv::Mat& tile, int mosaicId, const cv::Rect& box);
    void exportMosaic(const std::string& path, double blending, cv::Mat mosaic);

private:
    struct TileData
    {
        ProbaUtils::Histogram<3> _histogram;
        ProbaUtils::GMMNDComponents<3> _gmm;
    };

private:
    std::shared_ptr<const Photo> _photo;
    const int _gridWidth;
    const int _gridHeight;
    const double _blendingStep;
    const double _blendingMin;
    const double _blendingMax;
};

