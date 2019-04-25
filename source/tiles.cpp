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
	cv::Point cropFirstPixelPos;
    cv::Size cropSize;
    cv::Mat tileMat(_tileSize, CV_8UC3, cv::Scalar(0, 0, 0));

    computeCropInfo(image.size(), cropFirstPixelPos, cropSize);
    computeTileImage(tileMat.data, image, cropFirstPixelPos, cropSize);
    MathTools::computeImageBGRFeatures(tileMat.data, _tileSize, cv::Point(0, 0), _tileSize.width, data.features, FEATURE_ROOT_SUBDIVISION);
    data.filename = filename;
	_tilesData.push_back(data);

	exportTile(tileMat, filename);
}

void Tiles::computeCropInfo(const cv::Size &imageSize, cv::Point & firstPixelPos, cv::Size &cropSize)
{
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

	firstPixelPos.x = (int)(floor((imageSize.width - cropSize.width) / 2.));
    if (imageRatio > 1.)
	    firstPixelPos.y = (int)(floor((imageSize.height  - cropSize.height) / 2.));
    else 
        firstPixelPos.y = (int)(floor((imageSize.height - cropSize.height) / 3.));
}

void Tiles::computeTileImage(uchar * tileData, const cv::Mat & image, const cv::Point & firstPixelPos, const cv::Size &cropSize)
{
    //Temporary data
    cv::Mat imageToProcess0(cropSize, CV_8UC3);
    cv::Mat imageToProcess1(cropSize, CV_8UC3);
    uchar *imageToProcessData[2] = { imageToProcess0.data, imageToProcess1.data };
    cv::Size currentSize[2] = { cropSize, cropSize };
    int sourceId = 0;
    int targetId = 1;

    //Copy cropped image
    uchar *imageData = image.data;
    int step = image.size().width;
    for (int i = 0; i < cropSize.height; i++) {
        for (int j = 0; j < cropSize.width; j++) {
            int imageId = MathTools::getDataIndex(firstPixelPos.y + i, firstPixelPos.x + j, step);
            int imageToProcessId = MathTools::getDataIndex(i, j, cropSize.width);
            for (int c = 0; c < 3; c++)
                imageToProcessData[sourceId][imageToProcessId + c] = imageData[imageId + c];
        }
    }

    //DEBUG
    int debugStep = 0;
    //DEBUG


    while (true) {
        double ratio = (double)currentSize[sourceId].width / (double)_tileSize.width;

        //DEBUG
        cv::Mat debugExport(currentSize[sourceId], CV_8UC3);
        for (int k = 0; k < 3 * currentSize[sourceId].width * currentSize[sourceId].height; k++) {
            debugExport.data[k] = imageToProcessData[sourceId][k];
        }
        std::vector<int> image_params;
        image_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        image_params.push_back(100);
        cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\Tile_step_" + std::to_string(debugStep) + ".jpg", debugExport, image_params);
        debugStep++;
        //DEBUG

        if (ratio == 1.)
            break;
        else if (ratio > 2.) {
            currentSize[targetId].width = (int)ceil(currentSize[sourceId].width / 2.);
            currentSize[targetId].height = (int)ceil(currentSize[sourceId].height / 2.);
        }
        else {
            currentSize[targetId] = _tileSize;
        }

        ratio = (double)currentSize[sourceId].width / (double)currentSize[targetId].width;

        for (int i = 0; i < currentSize[targetId].height; i++) {
            for (int j = 0; j < currentSize[targetId].width; j++) {
                int dataId = MathTools::getDataIndex(i, j, currentSize[targetId].width);
                computeBicubicInterpolationPixelColor(&imageToProcessData[targetId][dataId], imageToProcessData[sourceId], currentSize[sourceId], i, j, ratio);
            }
        }

        sourceId = (sourceId + 1) % 2;
        targetId = (targetId + 1) % 2;
    }

    //Copy result tile
    for (int k = 0; k < 3 * _tileSize.width * _tileSize.height; k++) {
        tileData[k] = imageToProcessData[sourceId][k];
    }
}

void Tiles::computeBicubicInterpolationPixelColor(uchar* pixel, const uchar *imageData, const cv::Size &size, int i, int j, double ratio)
{
    uchar pixelColorGrid[16];
    double x =  (double)j * ratio;
    double y =  (double)i * ratio;
    int iFirstGrid = (int)round(y) - 2;
    int jFirstGrid = (int)round(x) - 2;
     
    for (int color = 0; color < 3; color++) {
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                int dataId = MathTools::getClippedDataIndex(iFirstGrid + row, jFirstGrid + col, size.width, size);
                pixelColorGrid[col * 4 + row] = imageData[dataId + color];
            }
        }

        pixel[color] = MathTools::biCubicInterpolation(x - round(x) + 1, y - round(y) + 1, pixelColorGrid);
    }
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
