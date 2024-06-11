#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "IRegionOfInterest.h"
#include "Photo.h"


class ITiles
{
public:
    ITiles(const std::string& path) : _path(path), _tempPath(path + "PMG_temp") {};
    virtual ~ITiles() {};

public:
    virtual void initialize() = 0;
    virtual void getImage(int tileID, cv::Mat& image) const = 0;
    virtual unsigned int getNbTiles() const = 0;
    virtual void remove(std::vector<unsigned int>& toRemove) = 0;
    virtual void compute(const IRegionOfInterest& roi, const Photo& photo) = 0;
    virtual double computeSquareDistance(int i, int j, int tileID) const = 0;
    virtual const std::string getTileFilepath(int tileId) const = 0;

protected:
    const std::string _path;
    const std::string _tempPath;
};
