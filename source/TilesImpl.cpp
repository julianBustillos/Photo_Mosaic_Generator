#include "TilesImpl.h"
#include "CustomException.h"
#include "OutputManager.h"
#include "MathUtils.h"
#include "ProgressBar.h"
#include "Log.h"
#include <iostream>
#include <filesystem>
#include <thread>//TODO DEBUG


TilesImpl::TilesImpl(const std::string& path, int subdivisions) :
    ITiles(path), _tileParam({cv::IMWRITE_PNG_COMPRESSION, 0}), _subdivisions(subdivisions)
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
                _tilesData.emplace_back(data);
            }
        }
    }

    Log::Logger::getInstance().log(Log::TRACE) << _tilesData.size() << " tiles found.";
}

unsigned int TilesImpl::getNbTiles() const
{
    return _tilesData.size();
}

void TilesImpl::getImage(int tileID, cv::Mat& image) const
{
    image = cv::imread(_tilesData[tileID]._imagePath, cv::IMREAD_COLOR);
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

void TilesImpl::compute(const IRegionOfInterest& roi, const Photo& photo)
{
    cv::Mat image;
    removeTemp();
    createTemp();

    ProgressBar progressBar("Compute tile candidates ", 60);//TODO DEBUG
    progressBar.setNbSteps(_tilesData.size());
    std::thread barThread(&ProgressBar::threadExecution, &progressBar);//TODO DEBUG

    OutputManager::getInstance().cstderr_silent();
    const int padding = std::to_string(_tilesData.size()).length();
    for (int t = 0; t < _tilesData.size(); t++)
    {
        getImage(t, image);
        std::string index = std::to_string(t);
        index = std::string(padding - index.length(), '0') + index;
        _tilesData[t]._tilePath = _tempPath + "\\tile_" + index + ".png";
        computeTileFeatures(image, roi, photo.getTileSize(), _tilesData[t]);
        progressBar.addSteps(1);
    }
    OutputManager::getInstance().cstderr_restore();
    Log::Logger::getInstance().log(Log::TRACE) <<"Tiles features computed.";
    barThread.join();

    _photoFeatures.resize(_subdivisions * _subdivisions * NbFeatures);
    int f = 0;
    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++, f++)
        {
            double* features = &_photoFeatures[f * NbFeatures];
            MathUtils::computeImageBGRFeatures(photo.getImage(), photo.getTileBox(i, j, true), features, FeatureDiv, NbFeatures);
        }
    }
    Log::Logger::getInstance().log(Log::TRACE) << "Photo features computed.";
}

double TilesImpl::computeSquareDistance(int i, int j, int tileID) const
{
    const double* features = &_photoFeatures[(i * _subdivisions + j) * NbFeatures];
    return MathUtils::BGRFeatureDistance(features, _tilesData[tileID]._features, NbFeatures);
}

const std::string TilesImpl::getTileFilepath(int tileId) const
{
    return _tilesData[tileId]._tilePath;
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
        Log::Logger::getInstance().log(Log::TRACE) << _tempPath << " temporary folder created.";
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
        Log::Logger::getInstance().log(Log::TRACE) << _tempPath << " temporary folder removed.";
    }
}

void TilesImpl::computeTileFeatures(const cv::Mat& image, const IRegionOfInterest& roi, const cv::Size& tileSize, Data& data)
{
    cv::Rect box;
    cv::Mat tileMat;

    computeCropInfo(image, box, roi, tileSize);
    MathUtils::computeImageResampling(tileMat, tileSize, image, box, MathUtils::LANCZOS);
    MathUtils::computeImageBGRFeatures(tileMat, cv::Rect(0, 0, tileSize.width, tileSize.height), data._features, FeatureDiv, NbFeatures);
    exportTile(tileMat, data._tilePath);
}

void TilesImpl::computeCropInfo(const cv::Mat& image, cv::Rect& box, const IRegionOfInterest& roi, const cv::Size& tileSize)
{
    if (image.size() == tileSize)
    {
        box.x = box.y = 0;
        box.width = image.size().width;
        box.height = image.size().height;
        return;
    }

    double wScaleInv = (double)image.size().width / (double)tileSize.width;
    double hScaleInv = (double)image.size().height / (double)tileSize.height;
    double scaleInv = std::min(wScaleInv, hScaleInv);

    box.width = (int)ceil(tileSize.width * scaleInv);
    box.height = (int)ceil(tileSize.height * scaleInv);

    roi.find(image, box, wScaleInv < hScaleInv);
}

void TilesImpl::exportTile(const cv::Mat& tile, const std::string& tilePath)
{
    cv::imwrite(tilePath, tile, _tileParam);
    if (!std::filesystem::exists(tilePath))
        throw CustomException("Impossible to create temporary tile : " + tilePath, CustomException::Level::ERROR);
}

void TilesImpl::printInfo() const
{
    std::cout << "TILES :" << std::endl;
    std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
    std::cout << std::endl;
}
