#include "Photo.h"
#include "CustomException.h"
#include "OutputManager.h"
#include "ImageUtils.h"
#include "Log.h"


Photo::Photo(const std::string& path, std::tuple<int, int> grid, double scale, std::tuple<int, int, bool> resolution) :
    _filePath(path), _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid)), _scale(scale), _resolutionWidth(std::get<0>(resolution)), _resolutionHeight(std::get<1>(resolution)), _resolutionCrop(std::get<2>(resolution))
{
}

void Photo::initialize()
{
    OutputManager::get().cstderr_silent();
    cv::Mat inputImage = cv::imread(_filePath);
    OutputManager::get().cstderr_restore();
    if (!inputImage.data)
        throw CustomException("Impossible to load image : " + _filePath, CustomException::Level::ERROR);

    _inputSize = inputImage.size();
    cv::Size resampleSize = _inputSize;
    cv::Size targetSize = _inputSize;
    if (_scale > 0)
    {
        targetSize = resampleSize = cv::Size((int)((double)resampleSize.width * _scale), (int)((double)resampleSize.height * _scale));
    }
    else if (_resolutionWidth > 0 && _resolutionHeight > 0)
    {
        targetSize = resampleSize = cv::Size(_resolutionWidth, _resolutionHeight);
        if (_resolutionCrop)
        {
            double inputRatio = (double)_inputSize.width / (double)_inputSize.height;
            double targetRatio = (double)_resolutionWidth / (double)_resolutionHeight;
            if (inputRatio < targetRatio)
                resampleSize.height = (int)((double)targetSize.width / inputRatio);
            else if (inputRatio > targetRatio)
                resampleSize.width = (int)((double)targetSize.height * inputRatio);
        }
    }
    cv::Mat resampledPhoto;
    ImageUtils::resample(resampledPhoto, resampleSize, inputImage, ImageUtils::LANCZOS);

    _tileSize = cv::Size(targetSize.width / _gridWidth, targetSize.height / _gridHeight);
    if (_tileSize.width < MinTileSize || _tileSize.height < MinTileSize)
        throw CustomException("Image subdivision leads to tiles with " + std::to_string(_tileSize.width) + "*" + std::to_string(_tileSize.height) + " size (minimum is " + std::to_string(MinTileSize) + "*" + std::to_string(MinTileSize) + ")", CustomException::Level::ERROR);
    
    _croppedSize = cv::Size(resampledPhoto.cols - _gridWidth * _tileSize.width, resampledPhoto.rows - _gridHeight * _tileSize.height);
    const int nbTiles = _gridWidth * _gridHeight;
    _tiles.reserve(nbTiles);
    for (int mosaicId = 0; mosaicId < nbTiles; mosaicId++)
    {
        _tiles.emplace_back(_tileSize, CV_8UC3);
        computeTile(resampledPhoto, mosaicId);
    }

    Log::Logger::get().log(Log::INFO) << "Photo size  : " << _inputSize.width << "*" << _inputSize.height;
    Log::Logger::get().log(Log::INFO) << "Mosaic size : " << resampledPhoto.cols - _croppedSize.width << "*" << resampledPhoto.rows - _croppedSize.height;
    Log::Logger::get().log(Log::INFO) << "Cropped size   : " << _croppedSize.width << "*" << _croppedSize.height;
    Log::Logger::get().log(Log::INFO) << "Tile size   : " << _tileSize.width << "*" << _tileSize.height;
}

cv::Rect Photo::getTileBox(int mosaicId) const
{
    cv::Rect box;
    int i = mosaicId / _gridWidth;
    int j = mosaicId - i * _gridWidth;
    box.y = i * _tileSize.height;
    box.x = j * _tileSize.width;
    box.width = _tileSize.width;
    box.height = _tileSize.height;

    return box;
}

cv::Size Photo::getTileSize() const
{
    return _tileSize;
}

const cv::Mat& Photo::getTile(int mosaicId) const
{
    return _tiles[mosaicId];
}

std::string Photo::getDirectory() const
{
    int filenameIndex = (int)_filePath.find_last_of('\\');
    if (filenameIndex >= 0)
        return _filePath.substr(0, filenameIndex);
    return "";
}

void Photo::computeTile(const cv::Mat& photo, int mosaicId)
{
    const int photoWidth = photo.cols;
    cv::Rect box = getTileBox(mosaicId);
    box.y += _croppedSize.height / 2;
    box.x += _croppedSize.width / 2;

    int p = 3 * (box.y * photoWidth + box.x);
    const int step = 3 * (photoWidth - box.width);
    for (int i = 0, t = 0; i < box.height; i++, p += step)
    {
        for (int j = 0; j < box.width; j++)
        {
            for (int c = 0; c < 3; c++, t++, p++)
            {
                _tiles[mosaicId].data[t] = photo.data[p];
            }
        }
    }

}
