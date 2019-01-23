#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


class Tiles {
public:
	struct Data {
		std::string filename;
        double features[48];
	};

public:
	Tiles(const std::string &path, const cv::Size &tileSize);
	~Tiles();
    const std::vector<Data> &getTileDataVector() const;
    const std::string getTempTilePath() const;
	
private:
	void computeTileData(const cv::Mat &image, const std::string &filename);
	void computeCropInfo(const cv::Size &imageSize, cv::Point &firstPixelPos, double &ratio);
	void computeTilePixelColor(uchar* tilePixel, const cv::Mat &image, const cv::Size &imageSize, int i, int j, const cv::Point &firstPixelPos, double ratio);
    int getDataIndex(int i, int j, const cv::Size &matSize);
	void exportTile(const cv::Mat &tile, const std::string &filename);
	void printInfo() const;

private:
	std::string _tempPath;
	std::vector<Data> _tilesData;
	const cv::Size _tileSize;
};
