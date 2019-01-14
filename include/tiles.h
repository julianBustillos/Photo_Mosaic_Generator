#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


class Tiles {
private:
	struct Data {
		std::string filename;
	};

public:
	Tiles(const std::string &path, const cv::Size &tileSize);
	~Tiles();
	
private:
	void computeTileData(const cv::Mat &image, const std::string &filename);
	void computeCropInfo(const cv::Size &imageSize, cv::Point &firstPixelPos, double &ratio);
	uchar computeTilePixelValue(const cv::Mat &image, const unsigned int i, const unsigned int j, const unsigned int color, const cv::Point &firstPixelPos, const double ratio);
	void exportTile(const cv::Mat &tile, const std::string &filename);
	void printInfo();

private:
	std::string _tempPath;
	std::vector<Data> _tilesData;
	const cv::Size _tileSize;
};
