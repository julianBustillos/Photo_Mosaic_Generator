#pragma once

#include "variables.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


class Tiles {
public:
	struct Data {
		std::string filename;
        double features[3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION];
	};

public:
	Tiles(const std::string &path, const cv::Size &tileSize);
	~Tiles();
    const std::vector<Data> &getTileDataVector() const;
    const std::string getTempTilePath() const;
	
private:
	void computeTileData(const cv::Mat &image, const std::string &filename);
	void computeCropInfo(const cv::Mat &image, cv::Point &firstPixelPos, cv::Size &cropSize);
	void exportTile(const cv::Mat &tile, const std::string &filename);
	void printInfo() const;

private:
	std::string _tempPath;
	std::vector<Data> _tilesData;
	const cv::Size _tileSize;
};
