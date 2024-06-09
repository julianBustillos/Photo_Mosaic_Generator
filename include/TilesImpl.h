#pragma once

#include "ITiles.h"
#include <string>


class TilesImpl : public ITiles
{
private:
    static constexpr int FeatureDiv = 4;
    static constexpr int NbFeatures = 3 * FeatureDiv * FeatureDiv;

public:
    TilesImpl(const std::string& path, const cv::Size& tileSize, int subdivisions);
    virtual ~TilesImpl();

public:
    virtual void initialize();
    virtual unsigned int getNbTiles() const;
    virtual void getImage(int tileID, cv::Mat& image) const;
    virtual void remove(std::vector<unsigned int>& toRemove);
    virtual void compute(const IRegionOfInterest& roi, const Photo& photo);
    virtual double computeSquareDistance(int i, int j, int tileID) const;
    virtual const std::string getTileFilepath(int tileId) const;

private:
    struct Data
    {
        std::string _imagePath = "";
        std::string _tilePath = "";
        double _features[NbFeatures] = { 0 };
    };

private:
    bool checkExtension(const std::string& extension) const;
    void createTemp() const;
    void removeTemp() const;
    void computeTileFeatures(const cv::Mat& image, const IRegionOfInterest& roi, Data& data);
    void computeCropInfo(const cv::Mat& image, cv::Rect& box, const IRegionOfInterest& roi);
    void exportTile(const cv::Mat& tile, const std::string& tilePath);
    void printInfo() const;

private:
    const std::vector<int> _tileParam;
    const int _subdivisions;
    std::vector<Data> _tilesData;
    std::vector<double> _photoFeatures;
};