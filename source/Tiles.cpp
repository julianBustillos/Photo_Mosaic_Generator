#include "Tiles.h"
#include "CustomException.h"
#include "OutputManager.h"
#include "ImageUtils.h"
#include "ProgressBar.h"
#include "Log.h"
#include "Console.h"
#include <filesystem>
#include <omp.h>


const std::string Tiles::TempDir = "PMG_temp";

Tiles::Tiles(const std::string& path, std::tuple<int, int> grid) :
    _path(path), _tempPath(path + TempDir), _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid))
{
}

Tiles::~Tiles()
{
    removeTemp();
}

void Tiles::initialize(int minNbTiles)
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

    Log::Logger::get().log(Log::TRACE) << _tilesData.size() << " tiles found.";

    if (_tilesData.size() < minNbTiles)
        throw CustomException("No sufficient number of tiles, " + std::to_string(_tilesData.size()) + " found but should have at least " + std::to_string(minNbTiles), CustomException::ERROR);
}

unsigned int Tiles::getNbTiles() const
{
    return _tilesData.size();
}

void Tiles::getImage(int tileID, cv::Mat& image) const
{
    image = cv::imread(_tilesData[tileID]._imagePath, cv::IMREAD_COLOR);
}

void Tiles::remove(std::vector<unsigned int>& toRemove)
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

void Tiles::compute(const FaceDetectionROI& roi, const Photo& photo)
{
    removeTemp();
    createTemp();

    Console::Out::initBar("Computing tile candidates ", _tilesData.size());
    Console::Out::startBar(Console::DEFAULT);

    OutputManager::get().cstderr_silent();
    const int padding = std::to_string(_tilesData.size()).length();
    #pragma omp parallel for
    for (int t = 0; t < _tilesData.size(); t++)
    {
        cv::Mat image;
        getImage(t, image);
        std::string index = std::to_string(t);
        index = std::string(padding - index.length(), '0') + index;
        _tilesData[t]._tilePath = _tempPath + "\\tile_" + index + ".png";
        computeTileFeatures(image, roi, photo.getTileSize(), _tilesData[t], omp_get_thread_num());
        Console::Out::addBarSteps(1);
    }
    OutputManager::get().cstderr_restore();
    Log::Logger::get().log(Log::TRACE) <<"Tiles features computed.";
    Console::Out::waitBar();

    _photoFeatures.resize(_gridWidth * _gridHeight * NbFeatures);
    for (int mosaicId = 0; mosaicId < _gridWidth * _gridHeight; mosaicId++)
    {
        double* features = &_photoFeatures[mosaicId * NbFeatures];
        ImageUtils::computeFeatures(photo.getImage(), photo.getTileBox(mosaicId, true), features, FeatureDiv, NbFeatures);
    }
    Log::Logger::get().log(Log::TRACE) << "Photo features computed.";
}

double Tiles::computeDistance(int i, int j, int tileID) const
{
    const double* features = &_photoFeatures[(i * _gridWidth + j) * NbFeatures];
    return ImageUtils::featureDistance(features, _tilesData[tileID]._features, NbFeatures);
}

const std::string Tiles::getTileFilepath(int tileId) const
{
    return _tilesData[tileId]._tilePath;
}

bool Tiles::checkExtension(const std::string& extension) const
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

void Tiles::createTemp() const
{
    if (!std::filesystem::exists(_tempPath))
    {
        std::filesystem::create_directory(_tempPath);
        Log::Logger::get().log(Log::TRACE) << _tempPath << " temporary folder created.";
    }
    if (!std::filesystem::exists(_tempPath))
    {
        throw CustomException("Impossible to create directory : " + _tempPath, CustomException::Level::ERROR);
    }
}

void Tiles::removeTemp() const
{
    if (std::filesystem::exists(_tempPath))
    {
        std::filesystem::remove_all(_tempPath);
        Log::Logger::get().log(Log::TRACE) << _tempPath << " temporary folder removed.";
    }
}

void Tiles::computeTileFeatures(const cv::Mat& image, const FaceDetectionROI& roi, const cv::Size& tileSize, Data& data, int threadID)
{
    cv::Rect box;
    cv::Mat tileMat;

    computeCropInfo(image, box, roi, tileSize, threadID);
    ImageUtils::resample(tileMat, tileSize, image, box, ImageUtils::LANCZOS);
    ImageUtils::computeFeatures(tileMat, cv::Rect(0, 0, tileSize.width, tileSize.height), data._features, FeatureDiv, NbFeatures);
    exportTile(tileMat, data._tilePath);
}

void Tiles::computeCropInfo(const cv::Mat& image, cv::Rect& box, const FaceDetectionROI& roi, const cv::Size& tileSize, int threadID)
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

    roi.find(image, box, wScaleInv < hScaleInv, threadID);
}

void Tiles::exportTile(const cv::Mat& tile, const std::string& tilePath)
{
    cv::imwrite(tilePath, tile, std::vector<int>({TileParam[0], TileParam[1]}));
    if (!std::filesystem::exists(tilePath))
        throw CustomException("Impossible to create temporary tile : " + tilePath, CustomException::Level::ERROR);
}

