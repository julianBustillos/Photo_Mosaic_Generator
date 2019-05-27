#include "tiles.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "customException.h"
#include "mathTools.h"
#include "outputDisabler.h"
#include "regionOfInterest.h"


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
	cv::Point cropFirstPixelPos;
    cv::Size cropSize;
    cv::Mat tileMat(_tileSize, CV_8UC3, cv::Scalar(0, 0, 0));

    computeCropInfo(image, cropFirstPixelPos, cropSize);
    MathTools::computeImageResampling(tileMat.data, _tileSize, image.data, image.size(), cropFirstPixelPos, cropSize, BLUR_NB_BOXES);
    MathTools::computeImageBGRFeatures(tileMat.data, _tileSize, cv::Point(0, 0), _tileSize.width, data.features, FEATURE_ROOT_SUBDIVISION);
    data.filename = filename.substr(0, filename.find_last_of('.')) + ".png";
	_tilesData.push_back(data);

	exportTile(tileMat, data.filename);
}

void Tiles::computeCropInfo(const cv::Mat &image, cv::Point & firstPixelPos, cv::Size &cropSize)
{
    const cv::Size imageSize = image.size();

    if (imageSize == _tileSize) {
        firstPixelPos = cv::Point(0, 0);
        cropSize = imageSize;
        return;
    }

    double rWidth = (double)imageSize.width / (double)_tileSize.width;
	double rHeight = (double)imageSize.height / (double)_tileSize.height;
    double imageRatio = (double)imageSize.width / (double)imageSize.height;

	double ratio = std::min(rHeight, rWidth);
    cropSize.width = (int)ceil(_tileSize.width * ratio);
    cropSize.height = (int)ceil(_tileSize.height * ratio);

    RegionOfInterest roi(image); //TODO

	firstPixelPos.x = (int)(floor((imageSize.width - cropSize.width) / 2.));
    if (imageRatio > 1.)
	    firstPixelPos.y = (int)(floor((imageSize.height  - cropSize.height) / 2.));
    else 
        firstPixelPos.y = (int)(floor((imageSize.height - cropSize.height) / 3.));
}

void Tiles::exportTile(const cv::Mat & tile, const std::string & filename)
{
    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.push_back(0);
	cv::imwrite(_tempPath + filename, tile, image_params);
	if (!boost::filesystem::exists(_tempPath + filename))
		throw CustomException("Impossible to create temporary tile : " + _tempPath + filename, CustomException::Level::NORMAL);
}

void Tiles::printInfo() const
{
	std::cout << "TILES :" << std::endl;
	std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
	std::cout << std::endl;
}
