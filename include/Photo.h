#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <opencv2/opencv.hpp>


class Photo
{
private:
    static constexpr int MinTileSize = 32;

public:
    Photo(const std::string& path, std::tuple<int, int> grid, double scale, std::tuple<int, int, bool> resolution);
    ~Photo() {};

public:
    void initialize();
    cv::Rect getTileBox(int mosaicId) const;
    cv::Size getTileSize() const;
    const cv::Mat& getTile(int mosaicId) const;
    std::string getDirectory() const;

private:
    void computeTile(const cv::Mat& photo, int mosaicId);

private:
    const std::string _filePath;
    const int _gridWidth;
    const int _gridHeight;
    const double _scale;
    const int _resolutionWidth;
    const int _resolutionHeight;
    const bool _resolutionCrop;
    std::vector<cv::Mat> _tiles;
    cv::Size _inputSize;
    cv::Size _tileSize;
    cv::Size _croppedSize;
};