#include "tiles.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>


Tiles::Tiles(const std::string &path)
{
	for (auto it = boost::filesystem::directory_iterator(path); it != boost::filesystem::directory_iterator(); it++)
	{
		if (!is_directory(it->path())) {
			cv::Mat mat = cv::imread(it->path().generic().string(), cv::IMREAD_COLOR);
			if (!mat.data)
				continue;

			Data temp;
			temp.filename = it->path().filename().string();
			_tilesData.push_back(temp);
		}
	}

	printInfo();
}

void Tiles::printInfo()
{
	std::cout << "TILES :" << std::endl;
	std::cout << "Number of tile found : " << _tilesData.size() << std::endl;
	std::cout << std::endl;
}
