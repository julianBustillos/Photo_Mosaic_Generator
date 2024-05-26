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
    cv::Point getFirstPixel(int i, int j, bool offset) const;
    cv::Size getTileSize() const;
    const uchar* getData() const;
    int getStep() const;
    std::string getDirectory() const;

private:
    void printInfo() const;

private:
    std::string _directory;
    cv::Mat _mat;
    cv::Size _tileSize;
    cv::Size _lostSize;
    cv::Size _inSize;
};