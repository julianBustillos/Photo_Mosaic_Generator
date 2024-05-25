#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "IRegionOfInterest.h"
#include "Photo.h"


class ITiles
{
public:
    //TODO : ADD COMMENT
    ITiles(const std::string& path, const cv::Size& tileSize) : _path(path), _tempPath(path + "PMG_temp"), _tileSize(tileSize) {};
    virtual ~ITiles() {};

    //TODO : ADD COMMENT
    virtual void initialize() = 0;

    //TODO : ADD COMMENT
    virtual void getImage(int tileID, cv::Mat& image) const = 0;

    //TODO : ADD COMMENT
    virtual unsigned int getNbTiles() const = 0;

    //TODO : ADD COMMENT
    virtual void remove(std::vector<unsigned int>& toRemove) = 0;

    //TODO : ADD COMMENT
    virtual void compute(const IRegionOfInterest& roi) = 0;

    //TODO : ADD COMMENT
    virtual double computeSquareDistance(const Photo& photo, int i, int j, int tileID) const = 0;

    //TODO : ADD COMMENT
    virtual const std::string getTileFilepath(int tileId) const = 0;

protected:
    const std::string _path;
    const std::string _tempPath;
    const cv::Size _tileSize;
};
