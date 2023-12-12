#pragma once

#include "variables.h"
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "IRegionOfInterest.h"
#include "Photo.h"


class ITiles
{
public:
    //TODO : ADD COMMENT
    ITiles(const std::string& path, const cv::Size& tileSize) : _path(path), _tempPath(path + "temp\\"), _tileSize(tileSize) {};

    //TODO : ADD COMMENT
    virtual ~ITiles() {};

    //TODO : ADD COMMENT
    virtual void compute(const IRegionOfInterest& roi) = 0;

    //TODO : ADD COMMENT
    virtual void computeSquareDistanceVector(std::vector<double>& squareDistances, const Photo& photo, int i, int j) const = 0;

    //TODO : ADD COMMENT
    virtual const std::string getTileFilepath(int tileId) const = 0;

protected:
    const std::string _path;
    const std::string _tempPath;
    const cv::Size _tileSize;
};
