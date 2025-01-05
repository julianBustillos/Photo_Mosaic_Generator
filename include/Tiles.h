#pragma once

#include "Photo.h"
#include "IRegionOfInterest.h"
#include <vector>
#include <string>


class Tiles
{
private:
    static const std::string TempDir;
    static constexpr int FeatureDiv = 4;
    static constexpr int NbFeatures = 3 * FeatureDiv * FeatureDiv;

public:
    Tiles(const std::string& path, int subdivisions);
    ~Tiles();

public:
    void initialize(int minNbTiles);
    unsigned int getNbTiles() const;
    void getImage(int tileID, cv::Mat& image) const;
    void remove(std::vector<unsigned int>& toRemove);
    void compute(const IRegionOfInterest& roi, const Photo& photo);
    double computeDistance(int i, int j, int tileID) const;
    const std::string getTileFilepath(int tileId) const;

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
    void computeTileFeatures(const cv::Mat& image, const IRegionOfInterest& roi, const cv::Size& tileSize, Data& data, int threadID);
    void computeCropInfo(const cv::Mat& image, cv::Rect& box, const IRegionOfInterest& roi, const cv::Size& tileSize, int threadID);
    void exportTile(const cv::Mat& tile, const std::string& tilePath);

private:
    const std::string _path;
    const std::string _tempPath;
    const std::vector<int> _tileParam;
    const int _subdivisions;
    std::vector<Data> _tilesData;
    std::vector<double> _photoFeatures;
};
