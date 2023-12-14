#include "TilesImpl.h"
#include <iostream>
#include <filesystem>
#include "CustomException.h"
#include "Utils.h"
#include "outputDisabler.h"


TilesImpl::TilesImpl(const std::string& path, const cv::Size& tileSize) :
    ITiles(path, tileSize)
{
}

TilesImpl::~TilesImpl()
{
    if (std::filesystem::exists(_tempPath))
        std::filesystem::remove_all(_tempPath);
}

void TilesImpl::compute(const IRegionOfInterest& roi)
{
    if (!std::filesystem::exists(_tempPath))
        std::filesystem::create_directory(_tempPath);
    if (!std::filesystem::exists(_tempPath))
        throw CustomException("Impossible to create directory : " + _tempPath, CustomException::Level::NORMAL);

    for (auto it = std::filesystem::recursive_directory_iterator(_path); it != std::filesystem::recursive_directory_iterator(); it++)
    {
        if (!is_directory(it->path()))
        {
            START_DISABLE_STDERR
                cv::Mat image = cv::imread(it->path().string(), cv::IMREAD_COLOR);
            END_DISABLE_STDERR
                if (!image.data)
                    continue;
            computeTileData(image, it->path().filename().string(), roi);
        }
    }

    printInfo();
}

void TilesImpl::computeSquareDistanceVector(std::vector<double>& squareDistances, const Photo& photo, int i, int j) const
{
    squareDistances.resize(_tilesData.size(), -1);
    double features[3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION];
    cv::Point firstPixel = photo.getFirstPixel(i, j, true);

    Utils::computeImageBGRFeatures(photo.getData(), photo.getTileSize(), firstPixel, photo.getStep(), features, FEATURE_ROOT_SUBDIVISION);
    for (unsigned int t = 0; t < _tilesData.size(); t++)
    {
        squareDistances[t] = Utils::BGRFeatureDistance(features, _tilesData[t].features, FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION);
    }
}

const std::string TilesImpl::getTileFilepath(int tileId) const
{
    return _tempPath + _tilesData[tileId].filename;
}

void TilesImpl::computeTileData(const cv::Mat& image, const std::string& filename, const IRegionOfInterest& roi)
{
    Data data;
    cv::Point firstPixel;
    cv::Size cropSize;
    cv::Mat tileMat(_tileSize, CV_8UC3, cv::Scalar(0, 0, 0));

    computeCropInfo(image, firstPixel, cropSize, roi);
    Utils::computeImageResampling(tileMat, image, firstPixel, cropSize);
    Utils::computeImageBGRFeatures(tileMat.data, _tileSize, cv::Point(0, 0), _tileSize.width, data.features, FEATURE_ROOT_SUBDIVISION);
    data.filename = filename.substr(0, filename.find_last_of('.')) + ".png";
    _tilesData.push_back(data);

    exportTile(tileMat, data.filename);
}

void TilesImpl::computeCropInfo(const cv::Mat& image, cv::Point& firstPixel, cv::Size& cropSize, const IRegionOfInterest& roi)
{
    if (image.size() == _tileSize)
    {
        firstPixel = cv::Point(0, 0);
        cropSize = image.size();
        return;
    }

    double wScaleInv = (double)image.size().width / (double)_tileSize.width;
    double hScaleInv = (double)image.size().height / (double)_tileSize.height;
    double scaleInv = std::min(wScaleInv, hScaleInv);

    cropSize.width = (int)ceil(_tileSize.width * scaleInv);
    cropSize.height = (int)ceil(_tileSize.height * scaleInv);

    roi.find(image, firstPixel, cropSize, wScaleInv >= hScaleInv);
}

void TilesImpl::exportTile(const cv::Mat& tile, const std::string& filename)
{
    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.push_back(0);
    cv::imwrite(_tempPath + filename, tile, image_params);
    if (!std::filesystem::exists(_tempPath + filename))
        throw CustomException("Impossible to create temporary tile : " + _tempPath + filename, CustomException::Level::NORMAL);
}

void TilesImpl::printInfo() const
{
    std::cout << "TILES :" << std::endl;
    std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
    std::cout << std::endl;
}
