#include "Photo.h"
#include "CustomException.h"
#include "MathUtils.h"
#include <iostream>


Photo::Photo(const std::string& path, double scale, double ratio, int subdivision) :
    _filePath(path), _scale(scale), _ratio(ratio), _subdivision(subdivision)
{
}

void Photo::initialize()
{
    cv::Mat inputImage = cv::imread(_filePath, cv::IMREAD_COLOR);
    if (!inputImage.data)
        throw CustomException("Impossible to load image : " + _filePath, CustomException::Level::ERROR);

    _inSize = inputImage.size();
    double inputRatio = (double)_inSize.width / (double)_inSize.height;
    cv::Size croppedSize = _inSize;
    if (_ratio > 0.)
    {
        if (inputRatio < _ratio)
        {
            croppedSize.height = (double)_inSize.width / _ratio;
        }
        else if (inputRatio > _ratio)
        {
            croppedSize.width = (double)_inSize.height * _ratio;
        }
    }

    cv::Size targetSize((double)croppedSize.width * _scale, (double)croppedSize.height * _scale);
    MathUtils::computeImageResampling(_mat, targetSize, inputImage, MathUtils::LANCZOS);

    _tileSize = cv::Size(_mat.size().width / _subdivision, _mat.size().height / _subdivision);
    _lostSize = cv::Size(_mat.size().width - _subdivision * _tileSize.width, _mat.size().height - _subdivision * _tileSize.height);

    if (_tileSize.width < MinTileSize || _tileSize.height < MinTileSize)
        throw CustomException("Image subdivision leads to tiles with " + std::to_string(_tileSize.width) + "*" + std::to_string(_tileSize.height) + " size (minimum is " + std::to_string(MinTileSize) + "*" + std::to_string(MinTileSize) + ")", CustomException::Level::ERROR);

    printInfo();
}

cv::Rect Photo::getTileBox(int i, int j, bool doShift) const
{
    cv::Rect box;
    box.y = i * _tileSize.height;
    box.x = j * _tileSize.width;
    if (doShift)
    {
        box.y += _lostSize.height / 2;
        box.x += _lostSize.width / 2;
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

void Photo::printInfo() const
{
    std::cout << "PHOTO DATA : " << std::endl;
    std::cout << "Photo size  : " << _inSize.width << "*" << _inSize.height << std::endl;
    std::cout << "Mosaic size : " << _mat.size().width << "*" << _mat.size().height << std::endl;
    std::cout << "Tile size   : " << _tileSize.width << "*" << _tileSize.height << std::endl;
    std::cout << "Lost size   : " << _lostSize.width << "*" << _lostSize.height << std::endl;
    std::cout << std::endl;
}
