#include "Photo.h"
#include "CustomException.h"
#include "MathUtils.h"
#include <iostream>


Photo::Photo(const std::string& path, double scale, double ratio, int subdivision) :
    _directory("")
{
    cv::Mat inputImage = cv::imread(path, cv::IMREAD_COLOR);
    if (!inputImage.data)
        throw CustomException("Impossible to load image : " + path, CustomException::Level::ERROR);
    _inSize = inputImage.size();

    double inputRatio = (double)_inSize.width / (double)_inSize.height;
    cv::Size croppedSize = _inSize;
    if (ratio > 0.)
    {
        if (inputRatio < ratio)
        {
            croppedSize.height = (double)_inSize.width / ratio;
        }
        else if (inputRatio > ratio)
        {
            croppedSize.width = (double)_inSize.height * ratio;
        }
    }

    cv::Size targetSize((double)croppedSize.width * scale, (double)croppedSize.height * scale);
    MathUtils::computeImageResampling(_mat, targetSize, inputImage, MathUtils::LANCZOS);

    int filenameIndex = (int)path.find_last_of('\\');
    if (filenameIndex >= 0)
        _directory = path.substr(0, filenameIndex);

    _tileSize = cv::Size(_mat.size().width / subdivision, _mat.size().height / subdivision);
    _lostSize = cv::Size(_mat.size().width - subdivision * _tileSize.width, _mat.size().height - subdivision * _tileSize.height);

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
    return _directory;
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
