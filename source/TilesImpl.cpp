#include "TilesImpl.h"
#include <iostream>
#include <filesystem>
#include "CustomException.h"
#include "MathUtils.h"
#include "OutputDisabler.h"


TilesImpl::TilesImpl(const std::string& path, const cv::Size& tileSize) :
    ITiles(path, tileSize)
{
}

TilesImpl::~TilesImpl()
{
    removeTemp();
}

void TilesImpl::initialize()
{
    Data data;
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
            if (checkExtension(it->path().extension().string()))
            {
                data._imagePath = it->path().string();
                std::string filename = it->path().filename().string();
                data._tileName = filename.substr(0, filename.find_last_of('.')) + ".png";
                _tilesData.emplace_back(data);
            }
        }
    }
}

unsigned int TilesImpl::getNbTiles() const
{
    return _tilesData.size();
}

void TilesImpl::remove(std::vector<unsigned int>& toRemove)
{
    std::sort(toRemove.begin(), toRemove.end());

    int t1 = 0, t2 = 0, r = 0;
    for (; t2 < _tilesData.size(); t1++, t2++)
    {
        while (r < toRemove.size() && t2 >= toRemove[r])
        {
            if (t2 == toRemove[r])
                t2++;
            r++;
        }
        if (t2 >= _tilesData.size())
        {
            break;
        }
        if (t2 > t1)
        {
            _tilesData[t1] = _tilesData[t2];
        }
    }
    _tilesData.resize(_tilesData.size() - (t2 - t1));
}

void TilesImpl::compute(const IRegionOfInterest& roi)
{
    OutputDisabler disabler;
    removeTemp();
    createTemp();

    for (Data& data : _tilesData)
    {
        disabler.start();
        cv::Mat image = cv::imread(data._imagePath, cv::IMREAD_COLOR);
        disabler.end();
        if (!image.data)
        {
            continue;
        }
        computeTileFeatures(image, roi, data);
    }
    printInfo();
}

double TilesImpl::computeSquareDistance(const Photo& photo, int i, int j, int tileID) const
{
    double features[3 * FeatureRootSubdivision * FeatureRootSubdivision];
    cv::Point firstPixel = photo.getFirstPixel(i, j, true);

    MathUtils::computeImageBGRFeatures(photo.getData(), photo.getTileSize(), firstPixel, photo.getStep(), features, FeatureRootSubdivision);
    return MathUtils::BGRFeatureDistance(features, _tilesData[tileID]._features, FeatureRootSubdivision * FeatureRootSubdivision);
}

const std::string TilesImpl::getTileFilepath(int tileId) const
{
    return _tempPath + "\\" + _tilesData[tileId]._tileName;
}

bool TilesImpl::checkExtension(const std::string& extension) const
{
    if (extension == ".bmp") return true;
    if (extension == ".dib") return true;
    if (extension == ".jpeg") return true;
    if (extension == ".jpg") return true;
    if (extension == ".jpe") return true;
    if (extension == ".jp2") return true;
    if (extension == ".png") return true;
    if (extension == ".webp") return true;
    if (extension == ".pbm") return true;
    if (extension == ".pgm") return true;
    if (extension == ".ppm") return true;
    if (extension == ".pxm") return true;
    if (extension == ".pnm") return true;
    if (extension == ".tiff") return true;
    if (extension == ".tif") return true;
    return false;
}

void TilesImpl::createTemp() const
{
    if (!std::filesystem::exists(_tempPath))
    {
        std::filesystem::create_directory(_tempPath);
    }
    if (!std::filesystem::exists(_tempPath))
    {
        throw CustomException("Impossible to create directory : " + _tempPath, CustomException::Level::ERROR);
    }
}

void TilesImpl::removeTemp() const
{
    if (std::filesystem::exists(_tempPath))
    {
        std::filesystem::remove_all(_tempPath);
    }
}

void TilesImpl::computeTileFeatures(const cv::Mat& image, const IRegionOfInterest& roi, Data& data)
{
    cv::Point firstPixel;
    cv::Size cropSize;
    cv::Mat tileMat;

    computeCropInfo(image, firstPixel, cropSize, roi);
    MathUtils::computeImageResampling(tileMat, _tileSize, image, firstPixel, cropSize);
    MathUtils::computeImageBGRFeatures(tileMat.data, _tileSize, cv::Point(0, 0), _tileSize.width, data._features, FeatureRootSubdivision);
    exportTile(tileMat, data._tileName);
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
    image_params.emplace_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.emplace_back(0);
    std::string filePath = _tempPath + "\\" + filename;
    cv::imwrite(filePath, tile, image_params);
    if (!std::filesystem::exists(filePath))
        throw CustomException("Impossible to create temporary tile : " + filePath, CustomException::Level::ERROR);
}

void TilesImpl::printInfo() const
{
    std::cout << "TILES :" << std::endl;
    std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
    std::cout << std::endl;
}
