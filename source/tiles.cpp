#include "tiles.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "customException.h"
#include "mathTools.h"
#include "outputDisabler.h"


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
            START_DISABLE_STDERR
			cv::Mat image = cv::imread(it->path().generic().string(), cv::IMREAD_COLOR);
            END_DISABLE_STDERR
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

const std::vector<Tiles::Data>& Tiles::getTileDataVector() const
{
    return _tilesData;
}

const std::string Tiles::getTempTilePath() const
{
    return _tempPath;
}

void Tiles::computeTileData(const cv::Mat & image, const std::string &filename)
{
	Data data;
	cv::Point firstPixelPos;
	double ratio;
    cv::Size imageSize = image.size();
    cv::Mat tileMat(_tileSize, CV_8UC3, cv::Scalar(0, 0, 0));

    if (_tileSize != imageSize) {
        computeCropInfo(imageSize, firstPixelPos, ratio);

        for (int i = 0; i < _tileSize.height; i++)
            for (int j = 0; j < _tileSize.width; j++)
                computeTilePixelColor(&tileMat.data[getDataIndex(i, j, _tileSize)], image, imageSize, i, j, firstPixelPos, ratio);
    }
    else {
        tileMat = image;
    }

    MathTools::computeImageBGRFeatures(tileMat.data, _tileSize.width, _tileSize.height, 0, 0, _tileSize.width, data.features, FEATURE_ROOT_SUBDIVISION);

    data.filename = filename;
	_tilesData.push_back(data);
	exportTile(tileMat, filename);
}

void Tiles::computeCropInfo(const cv::Size &imageSize, cv::Point & firstPixelPos, double & ratio)
{
    double rWidth = (double)imageSize.width / (double)_tileSize.width;
	double rHeight = (double)imageSize.height / (double)_tileSize.height;
    double imageRatio = (double)imageSize.width / (double)imageSize.height;

	ratio = std::min(rHeight, rWidth);
	firstPixelPos.x = (int)(floor((imageSize.width - ceil(ratio * _tileSize.width)) / 2.));
    if (imageRatio > 1.)
	    firstPixelPos.y = (int)(floor((imageSize.height  - ceil(ratio * _tileSize.height)) / 2.));
    else 
        firstPixelPos.y = (int)(floor((imageSize.height - ceil(ratio * _tileSize.height)) / 3.));
}

void Tiles::computeTilePixelColor(uchar* tilePixel, const cv::Mat &image, const cv::Size &imageSize, const int i, const int j, const cv::Point &firstPixelPos, const double ratio)
{
    uchar pixelColorGrid[16];
    uchar *imageData = image.data;
    double x = (double)firstPixelPos.x + j * ratio;
    double y = (double)firstPixelPos.y + i * ratio;
    int iFirstGrid = (int)floor(y) - 1;
    int jFirstGrid = (int)floor(x) - 1;
     
    for (int color = 0; color < 3; color++) {
        for (int col = 0; col < 4; col++)
            for (int row = 0; row < 4; row++)
                pixelColorGrid[col * 4 + row] = imageData[getDataIndex(iFirstGrid + row, jFirstGrid + col, imageSize) + color];

        tilePixel[color] = MathTools::biCubicInterpolation(x - floor(x), y - floor(y), pixelColorGrid);
    }
}

int Tiles::getDataIndex(int i, int j, const cv::Size &matSize)
{
    int iSafe = MathTools::clip<int>(i, 0, matSize.height - 1);
    int jSafe = MathTools::clip<int>(j, 0, matSize.width - 1);

    return 3 * (iSafe * matSize.width + jSafe);
}

void Tiles::exportTile(const cv::Mat & tile, const std::string & filename)
{
	cv::imwrite(_tempPath + filename, tile);
	if (!boost::filesystem::exists(_tempPath + filename))
		throw CustomException("Impossible to create temporary tile : " + _tempPath + filename, CustomException::Level::NORMAL);
}

void Tiles::printInfo() const
{
	std::cout << "TILES :" << std::endl;
	std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
	std::cout << std::endl;
}
