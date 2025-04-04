#pragma once

#include <string>
#include <tuple>
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
    cv::Rect getTileBox(int i, int j, bool doShift) const;
    cv::Size getTileSize() const;
    const cv::Mat& getImage() const;
    std::string getDirectory() const;

private:
    const std::string _filePath;
    const int _gridWidth;
    const int _gridHeight;
    const double _scale;
    const int _resolutionWidth;
    const int _resolutionHeight;
    const bool _resolutionCrop;
    cv::Mat _mat;
    cv::Size _tileSize;
    cv::Size _lostSize;
    cv::Size _inSize;
};