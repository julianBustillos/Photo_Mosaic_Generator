#include "tiles.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "customException.h"
#include "mathTools.h"


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
			computeTilePixelColor(&tileMatData[getDataIndex(i, j, tileMat)], image, i, j, firstPixelPos, ratio);

	_tilesData.push_back(data);
	exportTile(tileMat, filename);
}

void Tiles::computeCropInfo(const cv::Size &imageSize, cv::Point & firstPixelPos, double & ratio)
{
    double rWidth = (double)imageSize.width / (double)_tileSize.width;
	double rHeight = (double)imageSize.height / (double)_tileSize.height;

	ratio = std::min(rHeight, rWidth);
	firstPixelPos.x = (int)(floor((imageSize.width - ceil(ratio * _tileSize.width)) / 2.));
	firstPixelPos.y = (int)(floor((imageSize.height  - ceil(ratio * _tileSize.height)) / 2.));
}

void Tiles::computeTilePixelColor(uchar* tilePixel, const cv::Mat &image, const int i, const int j, const cv::Point &firstPixelPos, const double ratio)
{
    cv::Mat pointColorGrid(4, 4, CV_64F);
    double *pointColorGridData = (double *)pointColorGrid.data;
    uchar *imageData = image.data;
    double x = (double)firstPixelPos.x + j * ratio;
    double y = (double)firstPixelPos.y + i * ratio;
    int iFirstGrid = (int)floor(y) - 1;
    int jFirstGrid = (int)floor(x) - 1;
     
    for (int color = 0; color < 3; color++) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                pointColorGridData[i * 4 + j] = (double)imageData[getDataIndex(iFirstGrid + i, jFirstGrid + j, image) + color];

        tilePixel[color] = MathTools::biCubicInterpolation(x - floor(x), y - floor(y), pointColorGrid);
    }
}

int Tiles::getDataIndex(const int i, const int j, const cv::Mat & mat)
{
    int iSafe = MathTools::clipInt(i, 0, mat.size().height);
    int jSafe = MathTools::clipInt(j, 0, mat.size().width);

    return mat.channels() * (iSafe * mat.cols + jSafe);
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
