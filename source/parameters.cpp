#include "parameters.h"
#include "customException.h"
#include <boost/filesystem.hpp>


std::string Parameters::getHelp()
{
	return
		"HELP :\n"
		"Photo_Mosaic_Generator.exe -i C:\\myImage -p C:\\path_to_tiles -s 13\n\n"
		"AVAILABLE ARGUMENTS :\n"
		"-h : help\n"
		"-p path to tiles image folder\n"
		"-i path to image\n"
		"-s image subdivision (height and width)\n";
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

const std::string Parameters::getPhotoPath()
{
	return _photoPath;
}

const std::string Parameters::getTilesPath()
{
	return _tilesPath;
}

const int Parameters::getSubdivision()
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
	}
	else if (std::strcmp(parameter, "-i") == 0) {
		_photoPath = value ? value : "";
	}
	else if (std::strcmp(parameter, "-s") == 0) {
		_subdivision = value ? std::atoi(value) : 0;
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
}
