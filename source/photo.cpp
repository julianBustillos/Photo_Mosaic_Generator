#include "Photo.h"
#include "CustomException.h"
#include "OutputManager.h"
#include "MathUtils.h"
#include "Log.h"


Photo::Photo(const std::string& path, std::tuple<int, int> grid, double scale, std::tuple<int, int, bool> resolution) :
    _filePath(path), _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid)), _scale(scale), _resolutionWidth(std::get<0>(resolution)), _resolutionHeight(std::get<1>(resolution)), _resolutionCrop(std::get<2>(resolution))
{
}

void Photo::initialize()
{
    OutputManager::get().cstderr_silent();
    cv::Mat inputImage = cv::imread(_filePath, cv::IMREAD_COLOR);
    OutputManager::get().cstderr_restore();
    if (!inputImage.data)
        throw CustomException("Impossible to load image : " + _filePath, CustomException::Level::ERROR);

    _inSize = inputImage.size();
    cv::Size targetSize = _inSize;
    if (_scale > 0)
    {
        targetSize.width = (int)((double)targetSize.width * _scale);
        targetSize.height = (int)((double)targetSize.height * _scale);
    }
    else if (_resolutionWidth > 0 && _resolutionHeight > 0)
    {
        targetSize.width = _resolutionWidth;
        targetSize.height = _resolutionHeight;
        if (_resolutionCrop)
        {
            double inputRatio = (double)_inSize.width / (double)_inSize.height;
            double targetRatio = (double)_resolutionWidth / (double)_resolutionHeight;
            if (inputRatio < targetRatio)
            {
                targetSize.height = (int)((double)_inSize.width / targetRatio);
            }
            else if (inputRatio > targetRatio)
            {
                targetSize.width = (int)((double)_inSize.height * targetRatio);
            }
        }
    }
    MathUtils::computeImageResampling(_mat, targetSize, inputImage, MathUtils::LANCZOS);

    _tileSize = cv::Size(_mat.size().width / _gridWidth, _mat.size().height / _gridHeight);
    _croppedSize = cv::Size(_mat.size().width - _gridWidth * _tileSize.width, _mat.size().height - _gridHeight * _tileSize.height);

    if (_tileSize.width < MinTileSize || _tileSize.height < MinTileSize)
        throw CustomException("Image subdivision leads to tiles with " + std::to_string(_tileSize.width) + "*" + std::to_string(_tileSize.height) + " size (minimum is " + std::to_string(MinTileSize) + "*" + std::to_string(MinTileSize) + ")", CustomException::Level::ERROR);

    Log::Logger::get().log(Log::INFO) << "Photo size  : " << _inSize.width << "*" << _inSize.height;
    Log::Logger::get().log(Log::INFO) << "Mosaic size : " << _mat.size().width << "*" << _mat.size().height;
    Log::Logger::get().log(Log::INFO) << "Tile size   : " << _tileSize.width << "*" << _tileSize.height;
    Log::Logger::get().log(Log::INFO) << "Cropped size   : " << _croppedSize.width << "*" << _croppedSize.height;
}

cv::Rect Photo::getTileBox(int i, int j, bool doShift) const
{
    cv::Rect box;
    box.y = i * _tileSize.height;
    box.x = j * _tileSize.width;
    if (doShift)
    {
        box.y += _croppedSize.height / 2;
        box.x += _croppedSize.width / 2;
    }
    box.width = _tileSize.width;
    box.height = _tileSize.height;

    return box;
}

cv::Size Photo::getTileSize() const
{
    return _tileSize;
}

const cv::Mat& Photo::getImage() const
{
    return _mat;
}

std::string Photo::getDirectory() const
{
    int filenameIndex = (int)_filePath.find_last_of('\\');
    if (filenameIndex >= 0)
        return _filePath.substr(0, filenameIndex);
    return "";
}

