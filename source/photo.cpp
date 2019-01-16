#include "photo.h"
#include "customException.h"
#include <iostream>


Photo::Photo(const std::string &path, int subdivision)
{
	_mat = cv::imread(path, cv::IMREAD_COLOR);
	if (!_mat.data)
		throw CustomException("Impossible to load image : " + path + ", use -i option", CustomException::Level::NORMAL);

	_tileSize = cv::Size(_mat.size().width / subdivision, _mat.size().height / subdivision);
	_lostSize = cv::Size(_mat.size().width - subdivision * _tileSize.width, _mat.size().height - subdivision * _tileSize.height);

	printData();
}

cv::Point Photo::getFirstPixel(int i, int j) const
{
	cv::Point position;
	position.x = _lostSize.height / 2 + i * _tileSize.height;
	position.y = _lostSize.width / 2 + i * _tileSize.width;
	return position;
}

cv::Size Photo::getTileSize() const
{
	return _tileSize;
}

void Photo::printData() const
{
	std::cout << "PHOTO DATA : " << std::endl;
	std::cout << "Photo size : " << _mat.size().height << "*" << _mat.size().width << std::endl;
	std::cout << "Tile size  : " << _tileSize.height << "*" << _tileSize.width << std::endl;
	std::cout << "Lost size  : " << _lostSize.height << "*" << _lostSize.width << std::endl;
	std::cout << std::endl;
}
