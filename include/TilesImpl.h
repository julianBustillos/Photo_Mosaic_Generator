#pragma once

#include "ITiles.h"


class TilesImpl : public ITiles
{
public:
    TilesImpl(const std::string &path, const cv::Size &tileSize);
    virtual ~TilesImpl();
    virtual void computeSquareDistanceVector(std::vector<double> &squareDistances, const Photo &photo, int i, int j) const;
    virtual const std::string getTileFilepath(int tileId) const;

private:
    struct Data {
        std::string filename;
        double features[3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION];
    };

private:
    void computeTileData(const cv::Mat &image, const std::string &filename);
    void computeCropInfo(const cv::Mat &image, cv::Point &firstPixelPos, cv::Size &cropSize);
    void exportTile(const cv::Mat &tile, const std::string &filename);
    void printInfo() const;

private:
    std::vector<Data> _tilesData;
};