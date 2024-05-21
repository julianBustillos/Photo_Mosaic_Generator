#pragma once

#include "ITiles.h"


class TilesImpl : public ITiles
{
private:
    static constexpr int FeatureRootSubdivision = 4;

public:
    TilesImpl(const std::string& path, const cv::Size& tileSize);
    virtual ~TilesImpl();
    virtual unsigned int getNbTiles() const;
    virtual void compute(const IRegionOfInterest& roi);
    virtual double computeSquareDistance(const Photo& photo, int i, int j, int tileID) const;
    virtual const std::string getTileFilepath(int tileId) const;

private:
    struct Data
    {
        std::string filename;
        double features[3 * FeatureRootSubdivision * FeatureRootSubdivision];
    };

private:
    void createTemp() const;
    void removeTemp() const;
    void computeTileData(const cv::Mat& image, const std::string& filename, const IRegionOfInterest& roi);
    void computeCropInfo(const cv::Mat& image, cv::Point& firstPixel, cv::Size& cropSize, const IRegionOfInterest& roi);
    void exportTile(const cv::Mat& tile, const std::string& filename);
    void printInfo() const;

private:
    std::vector<Data> _tilesData;
};