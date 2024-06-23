#pragma once

#include <string>
#include <opencv2/opencv.hpp>


class Photo
{
private:
    static constexpr int MinTileSize = 32;

public:
    Photo(const std::string& path, double scale, double ratio, int subdivision);
    ~Photo() {};

public:
    void initialize();
    cv::Rect getTileBox(int i, int j, bool doShift) const;
    cv::Size getTileSize() const;
    const cv::Mat& getImage() const;
    std::string getDirectory() const;

private:
    const std::string _filePath;
    const double _scale;
    const double _ratio;
    const int _subdivision;
    cv::Mat _mat;
    cv::Size _tileSize;
    cv::Size _lostSize;
    cv::Size _inSize;
};