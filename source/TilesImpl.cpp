#include "TilesImpl.h"
#include <iostream>
#include <filesystem>
#include "CustomException.h"
#include "Utils.h"
#include "OutputDisabler.h"


TilesImpl::TilesImpl(const std::string& path, const cv::Size& tileSize) :
    ITiles(path, tileSize)
{
}

TilesImpl::~TilesImpl()
{
    removeTemp();
}

void TilesImpl::compute(const IRegionOfInterest& roi)
{
    OutputDisabler disabler;
    removeTemp();
    createTemp();

    for (auto it = std::filesystem::recursive_directory_iterator(_path); it != std::filesystem::recursive_directory_iterator(); it++)
    {
        if (is_directory(it->path()))
        {
            if (it->path() == _tempPath)
            {
                it.disable_recursion_pending();
            }
        }
        else
        {
            disabler.start();
            cv::Mat image = cv::imread(it->path().string(), cv::IMREAD_COLOR);
             disabler.end();
             if (!image.data)
             {
                 continue;
             }
            computeTileData(image, it->path().filename().string(), roi);
        }
    }
    printInfo();
}

void TilesImpl::computeSquareDistanceVector(std::vector<double>& squareDistances, const Photo& photo, int i, int j) const
{
    squareDistances.resize(_tilesData.size(), -1);
    double features[3 * FeatureRootSubdivision * FeatureRootSubdivision];
    cv::Point firstPixel = photo.getFirstPixel(i, j, true);

    Utils::computeImageBGRFeatures(photo.getData(), photo.getTileSize(), firstPixel, photo.getStep(), features, FeatureRootSubdivision);
    for (unsigned int t = 0; t < _tilesData.size(); t++)
    {
        squareDistances[t] = Utils::BGRFeatureDistance(features, _tilesData[t].features, FeatureRootSubdivision * FeatureRootSubdivision);
    }
}

const std::string TilesImpl::getTileFilepath(int tileId) const
{
    return _tempPath + "\\" + _tilesData[tileId].filename;
}

void TilesImpl::createTemp() const
{
    if (!std::filesystem::exists(_tempPath))
    {
        std::filesystem::create_directory(_tempPath);
    }
    if (!std::filesystem::exists(_tempPath))
    {
        throw CustomException("Impossible to create directory : " + _tempPath, CustomException::Level::NORMAL);
    }
}

void TilesImpl::removeTemp() const
{
    if (std::filesystem::exists(_tempPath))
    {
        std::filesystem::remove_all(_tempPath);
    }
}

void TilesImpl::computeTileData(const cv::Mat& image, const std::string& filename, const IRegionOfInterest& roi)
{
    Data data;
    cv::Point firstPixel;
    cv::Size cropSize;
    cv::Mat tileMat;

    computeCropInfo(image, firstPixel, cropSize, roi);
    Utils::computeImageResampling(tileMat, _tileSize, image, firstPixel, cropSize);
    Utils::computeImageBGRFeatures(tileMat.data, _tileSize, cv::Point(0, 0), _tileSize.width, data.features, FeatureRootSubdivision);
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

    roi.find(image, firstPixel, cropSize, wScaleInv < hScaleInv);
}

void TilesImpl::exportTile(const cv::Mat& tile, const std::string& filename)
{
    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.push_back(0);
    std::string filePath = _tempPath + "\\" + filename;
    cv::imwrite(filePath, tile, image_params);
    if (!std::filesystem::exists(filePath))
        throw CustomException("Impossible to create temporary tile : " + filePath, CustomException::Level::NORMAL);
}

void TilesImpl::printInfo() const
{
    std::cout << "TILES :" << std::endl;
    std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
    std::cout << std::endl;
}
