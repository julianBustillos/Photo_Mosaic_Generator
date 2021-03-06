#include "Parameters.h"
#include "CustomException.h"
#include <boost/filesystem.hpp>
#include <algorithm>


std::string Parameters::getHelp()
{
	return
		"HELP :\n"
        "Photo_Mosaic_Generator.exe -i C:\\myImage -p C:\\path_to_tiles -s 13 [-rw 1920 -rh 1080]\n\n"
		"AVAILABLE ARGUMENTS :\n"
		"-h : help\n"
		"-p path to tiles image folder\n"
		"-i path to image\n"
		"-s image subdivision (height and width)\n"
        "-rw result width\n"
        "-rh result height\n";
}

Parameters::Parameters(int argc, char * argv[])
{
	for (int i = 1; i < argc; i += 2) {
		char *parameter = argv[i];
		char *value = ((i + 1) < argc) ? argv[i + 1] : NULL;
		parseArgument(parameter, value);
	}
	checkParsing();
}

std::string Parameters::getPhotoPath() const
{
	return _photoPath;
}

std::string Parameters::getTilesPath() const
{
	return _tilesPath;
}

int Parameters::getWidth() const
{
    return _width;
}

int Parameters::getHeight() const
{
    return _height;
}

int Parameters::getSubdivision() const
{
	return _subdivision;
}

void Parameters::parseArgument(char *parameter, char *value)
{
	if (std::strcmp(parameter, "-h") == 0) {
		throw CustomException("Help", CustomException::Level::HELP);
	}
	else if (std::strcmp(parameter, "-p") == 0) {
		_tilesPath = value ? value : "";
		std::replace(_tilesPath.begin(), _tilesPath.end(), '/', '\\');
		if (_tilesPath.back() != '\\')
			_tilesPath += "\\";
	}
	else if (std::strcmp(parameter, "-i") == 0) {
		_photoPath = value ? value : "";
		std::replace(_photoPath.begin(), _photoPath.end(), '/', '\\');
	}
	else if (std::strcmp(parameter, "-s") == 0) {
		_subdivision = value ? std::atoi(value) : 0;
	}
    else if (std::strcmp(parameter, "-rw") == 0) {
        _width = value ? std::atoi(value) : 0;
    }
    else if (std::strcmp(parameter, "-rh") == 0) {
        _height = value ? std::atoi(value) : 0;
    }
	else {
		std::string message = "Invalid parameter : ";
		message += parameter;
		throw CustomException(message, CustomException::Level::NORMAL);
	}
}

void Parameters::checkParsing()
{
	if (_tilesPath == "")
		throw CustomException("No path defined, use -p option", CustomException::Level::NORMAL);
	else if (!boost::filesystem::exists(_tilesPath)) {
		std::string message = "Invalid path : ";
		message += _tilesPath;
		message += ", use -p option";
		throw CustomException(message, CustomException::Level::NORMAL);
	}
	if (_photoPath == "")
		throw CustomException("No image defined, use -i option", CustomException::Level::NORMAL);
	else if (!boost::filesystem::exists(_photoPath)) {
		std::string message = "Invalid file : ";
		message += _photoPath;
		message += ", use -i option";
		throw CustomException(message, CustomException::Level::NORMAL);
	}
	if (_subdivision == 0)
		throw CustomException("Invalid subdivision value, use -s option", CustomException::Level::NORMAL);
    if (_width != 0 && _width < _subdivision) {
        throw CustomException("Invalid result width value, use -rw option", CustomException::Level::NORMAL);
    }
    if (_height != 0 && _height < _subdivision) {
        throw CustomException("Invalid result height value, use -rh option", CustomException::Level::NORMAL);
    }
}
