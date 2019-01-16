#pragma once

#include <string>
#include <opencv2/opencv.hpp>


class Photo {
public:
	Photo(const std::string &path, int subdivision);
	~Photo() {};
	cv::Point getFirstPixel(int i, int j) const;
	cv::Size getTileSize() const;

private:
	void printData() const;

private:
	cv::Mat _mat;
	cv::Size _tileSize;
	cv::Size _lostSize;
};