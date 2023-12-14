#include "Photo.h"
#include "CustomException.h"
#include "Utils.h"
#include "variables.h"
#include <iostream>


Photo::Photo(const std::string& path, int width, int height, int subdivision) :
    _directory("")
{
    cv::Mat originalImage = cv::imread(path, cv::IMREAD_COLOR);
    if (!originalImage.data)
        throw CustomException("Impossible to load image : " + path + ", use -i option", CustomException::Level::NORMAL);
    _oldSize = originalImage.size();

    if (width != 0 && height != 0)
    {
        Utils::computeImageResampling(_mat, cv::Size(width, height), originalImage, cv::Point(0, 0), originalImage.size());
    }
    else
    {
        _mat = originalImage;
    }

    int filenameIndex = (int)path.find_last_of('\\');
    if (filenameIndex >= 0)
        _directory = path.substr(0, filenameIndex);

    _tileSize = cv::Size(_mat.size().width / subdivision, _mat.size().height / subdivision);
    _lostSize = cv::Size(_mat.size().width - subdivision * _tileSize.width, _mat.size().height - subdivision * _tileSize.height);

    int minimumSize = 20; //TODO CONSTANT
    if (_tileSize.width < minimumSize || _tileSize.height < minimumSize)
        throw CustomException("Image subdivision leads to tiles with " + std::to_string(_tileSize.width) + "*" + std::to_string(_tileSize.height) + " size (minimum is " + std::to_string(minimumSize) + "*" + std::to_string(minimumSize) + ")", CustomException::Level::NORMAL);

    printInfo();
}

cv::Point Photo::getFirstPixel(int i, int j, bool offset) const
{
    cv::Point position;
    position.y = i * _tileSize.height;
    position.x = j * _tileSize.width;
    if (offset)
    {
        position.y += _lostSize.height / 2;
        position.x += _lostSize.width / 2;
    }

    return position;
}

cv::Size Photo::getTileSize() const
{
    return _tileSize;
}

const uchar* Photo::getData() const
{
    return _mat.data;
}

int Photo::getStep() const
{
    return _mat.size().width;
}

std::string Photo::getDirectory() const
{
    return _directory;
}

void Photo::printInfo() const
{
    std::cout << "PHOTO DATA : " << std::endl;
    std::cout << "Photo size  : " << _oldSize.width << "*" << _oldSize.height << std::endl;
    std::cout << "Mosaic size : " << _mat.size().width << "*" << _mat.size().height << std::endl;
    std::cout << "Tile size   : " << _tileSize.width << "*" << _tileSize.height << std::endl;
    std::cout << "Lost size   : " << _lostSize.width << "*" << _lostSize.height << std::endl;
    std::cout << std::endl;
}
