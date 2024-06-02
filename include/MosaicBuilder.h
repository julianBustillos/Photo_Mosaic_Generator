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
    MosaicBuilder(std::shared_ptr<const Photo> photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};
    void build(const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver);
    ~MosaicBuilder() { _photo.reset(); };

private:
    void copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Rect& box);
    void exportMosaic(const std::string& path, cv::Mat mosaic);
    void printInfo() const;

private:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};

