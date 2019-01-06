#include "parameters.h"
#include "customException.h"
#include <sys/stat.h>


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

void Parameters::parse(int argc, char * argv[])
{
	for (int i = 1; i < argc; i += 2) {
		char *parameter = argv[i];
		char *value = ((i + 1) < argc) ? argv[i + 1] : NULL;
		getArgument(parameter, value);
	}
	checkParsing();
}

void Parameters::getArgument(char *parameter, char *value)
{
	if (std::strcmp(parameter, "-h") == 0) {
		throw CustomException("Help", CustomException::Level::HELP);
	}
	else if (std::strcmp(parameter, "-p") == 0) {
		imageDirectoryPath = value ? value : "";
	}
	else if (std::strcmp(parameter, "-i") == 0) {
		mainImagePath = value ? value : "";
	}
	else if (std::strcmp(parameter, "-s") == 0) {
		subdivision = value ? std::atoi(value) : 0;
	}
	else {
		std::string message = "Invalid parameter : ";
		message += parameter;
		throw CustomException(message, CustomException::Level::NORMAL);
	}
}

void Parameters::checkParsing()
{
	if (imageDirectoryPath == "")
		throw CustomException("No path defined, use -p option", CustomException::Level::NORMAL);
	else if (!pathExist(imageDirectoryPath)) {
		std::string message = "Invalid path : ";
		message += imageDirectoryPath;
		message += ", use -p option";
		throw CustomException(message, CustomException::Level::NORMAL);
	}
	if (mainImagePath == "")
		throw CustomException("No image defined, use -i option", CustomException::Level::NORMAL);
	else if (!pathExist(mainImagePath)) {
		std::string message = "Invalid file : ";
		message += mainImagePath;
		message += ", use -i option";
		throw CustomException(message, CustomException::Level::NORMAL);
	}
	if (subdivision == 0)
		throw CustomException("Invalid subdivision value, use -s option", CustomException::Level::NORMAL);
}

bool Parameters::pathExist(std::string &path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}
