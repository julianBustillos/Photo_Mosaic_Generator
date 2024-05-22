#pragma once

#include "ITiles.h"
#include <string>


class TilesImpl : public ITiles
{
private:
    static constexpr int FeatureRootSubdivision = 4;

public:
    TilesImpl(const std::string& path, const cv::Size& tileSize);
    virtual ~TilesImpl();
    virtual void initialize();
    virtual unsigned int getNbTiles() const;
    virtual void remove(std::vector<unsigned int>& toRemove);
    virtual void compute(const IRegionOfInterest& roi);
    virtual double computeSquareDistance(const Photo& photo, int i, int j, int tileID) const;
    virtual const std::string getTileFilepath(int tileId) const;

private:
    struct Data
    {
        std::string _imagePath = "";
        std::string _tileName = "";
        double _features[3 * FeatureRootSubdivision * FeatureRootSubdivision] = { 0 };
    };

private:
    bool checkExtension(const std::string& extension) const;
    void createTemp() const;
    void removeTemp() const;
    void computeTileFeatures(const cv::Mat& image, const IRegionOfInterest& roi, Data& data);
    void computeCropInfo(const cv::Mat& image, cv::Point& firstPixel, cv::Size& cropSize, const IRegionOfInterest& roi);
    void exportTile(const cv::Mat& tile, const std::string& filename);
    void printInfo() const;

private:
    std::vector<Data> _tilesData;
};