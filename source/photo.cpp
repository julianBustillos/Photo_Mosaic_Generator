#include "photo.h"
#include "customException.h"
#include <iostream>


Photo::Photo(const std::string &path, int subdivision) :
    _directory("")
{
	_mat = cv::imread(path, cv::IMREAD_COLOR);
	if (!_mat.data)
		throw CustomException("Impossible to load image : " + path + ", use -i option", CustomException::Level::NORMAL);

    int filenameIndex = (int)path.find_last_of('\\');
    if (filenameIndex >= 0)
        _directory = path.substr(0, filenameIndex);

	_tileSize = cv::Size(_mat.size().width / subdivision, _mat.size().height / subdivision);
	_lostSize = cv::Size(_mat.size().width - subdivision * _tileSize.width, _mat.size().height - subdivision * _tileSize.height);

    int minimumSize = 20;
    if (_tileSize.width < minimumSize || _tileSize.height < minimumSize)
        throw CustomException("Image subdivision leads to tiles with " + std::to_string(_tileSize.width) + "*" + std::to_string(_tileSize.height) + " size (minimum is " + std::to_string(minimumSize) + "*" + std::to_string(minimumSize) + ")", CustomException::Level::NORMAL);

	printInfo();
}

const cv::Point Photo::getFirstPixel(int i, int j, bool offset) const
{
	cv::Point position;
	position.y = i * _tileSize.height;
	position.x = j * _tileSize.width;
    if (offset) {
        position.y += _lostSize.height / 2;
        position.x += _lostSize.width / 2;
    }

	return position;
}

const cv::Size Photo::getTileSize() const
{
	return _tileSize;
}

const uchar * Photo::getData() const
{
    return _mat.data;
}

const int Photo::getStep() const
{
    return _mat.size().width;
}

const std::string Photo::getDirectory() const
{
    return _directory;
}

void Photo::printInfo() const
{
	std::cout << "PHOTO DATA : " << std::endl;
	std::cout << "Photo size : " << _mat.size().width << "*" << _mat.size().height << std::endl;
	std::cout << "Tile size  : " << _tileSize.width << "*" << _tileSize.height << std::endl;
	std::cout << "Lost size  : " << _lostSize.width << "*" << _lostSize.height << std::endl;
	std::cout << std::endl;
}
