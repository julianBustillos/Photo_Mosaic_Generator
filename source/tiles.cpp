#include "tiles.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "customException.h"


Tiles::Tiles(const std::string &path, const cv::Size &tileSize) :
	_tileSize(tileSize)
{
	_tempPath = path + "temp\\";
	if (!boost::filesystem::exists(_tempPath))
		boost::filesystem::create_directory(_tempPath);
	if (!boost::filesystem::exists(_tempPath))
		throw CustomException("Impossible to create directory : " + _tempPath, CustomException::Level::NORMAL);

	for (auto it = boost::filesystem::directory_iterator(path); it != boost::filesystem::directory_iterator(); it++)
	{
		if (!is_directory(it->path())) {
			cv::Mat image = cv::imread(it->path().generic().string(), cv::IMREAD_COLOR);
			if (!image.data)
				continue;

			computeTileData(image, it->path().filename().string());
		}
	}

	printInfo();
}

Tiles::~Tiles()
{
	if (boost::filesystem::exists(_tempPath))
		boost::filesystem::remove_all(_tempPath);
}

void Tiles::computeTileData(const cv::Mat & image, const std::string &filename)
{
	Data data;
	cv::Point firstPixelPos;
	double ratio;

	computeCropInfo(image.size(), firstPixelPos, ratio);
	data.filename = filename;

	cv::Mat tileMat(_tileSize, CV_8UC3 , cv::Scalar(0, 0, 0));
	uchar *tileMatData = tileMat.data;
    for (int i = 0; i < _tileSize.height; i++)
        for (int j = 0; j < _tileSize.width; j++)
			for (int color = 0; color < 3; color++)
				tileMatData[3 * (i * tileMat.cols + j) + color] = computeTilePixelValue(image, i, j, color, firstPixelPos, ratio);

	_tilesData.push_back(data);
	exportTile(tileMat, filename);
}

void Tiles::computeCropInfo(const cv::Size &imageSize, cv::Point & firstPixelPos, double & ratio)
{
	double rHeight = (double)imageSize.height / (double)_tileSize.height;
	double rWidth  = (double)imageSize.width  / (double)_tileSize.width;

	ratio = std::min(rHeight, rWidth);
	firstPixelPos.x = (int)((imageSize.height - ratio * _tileSize.height) / 2.);
	firstPixelPos.y = (int)((imageSize.width  - ratio * _tileSize.width) / 2.);

	std::cout << firstPixelPos << std::endl;
	std::cout << ratio << std::endl;
}

uchar Tiles::computeTilePixelValue(const cv::Mat &image, const unsigned int i, const unsigned int j, const unsigned int colorId, const cv::Point &firstPixelPos, const double ratio)
{
    //Todo bicubic interpolation
	return 137;
}

void Tiles::exportTile(const cv::Mat & tile, const std::string & filename)
{
	cv::imwrite(_tempPath + filename, tile);
	if (!boost::filesystem::exists(_tempPath + filename))
		throw CustomException("Impossible to create temporary tile : " + _tempPath + filename, CustomException::Level::NORMAL);
}

void Tiles::printInfo()
{
	std::cout << "TILES :" << std::endl;
	std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
	std::cout << std::endl;
}
