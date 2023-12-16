#include "Parameters.h"
#include "CustomException.h"
#include <filesystem>
#include <algorithm>


std::string Parameters::getHelp()
{
    return
        "HELP :\n"
        "Photo_Mosaic_Generator.exe -i C:\\myImage -p C:\\path_to_tiles -d 13 [-s 1.7 -r 3.33]\n\n"
        "AVAILABLE ARGUMENTS :\n"
        "-h : help\n"
        "-p path to tiles image folder\n"
        "-i path to image\n"
        "-d image subdivision (height and width)\n"
        "-s image scale\n"
        "-r image ratio (width / height)\n";
}

Parameters::Parameters(int argc, char* argv[])
{
    for (int i = 1; i < argc; i += 2)
    {
        char* parameter = argv[i];
        char* value = ((i + 1) < argc)?argv[i + 1]:NULL;
        parse(parameter, value);
    }
    check();
}

std::string Parameters::getPhotoPath() const
{
    return _photoPath;
}

std::string Parameters::getTilesPath() const
{
    return _tilesPath;
}

double Parameters::getScale() const
{
    return _scale;
}

double Parameters::getRatio() const
{
    return _ratio;
}

int Parameters::getSubdivision() const
{
    return _subdivision;
}

void Parameters::parse(char* parameter, char* value)
{
    if (std::strcmp(parameter, "-h") == 0)
    {
        throw CustomException("Help", CustomException::Level::HELP);
    }
    else if (std::strcmp(parameter, "-p") == 0)
    {
        _tilesPath = value?value:"";
        std::replace(_tilesPath.begin(), _tilesPath.end(), '/', '\\');
        if (_tilesPath.back() != '\\')
            _tilesPath += "\\";
    }
    else if (std::strcmp(parameter, "-i") == 0)
    {
        _photoPath = value?value:"";
        std::replace(_photoPath.begin(), _photoPath.end(), '/', '\\');
    }
    else if (std::strcmp(parameter, "-d") == 0)
    {
        _subdivision = value?std::atoi(value):0;
    }
    else if (std::strcmp(parameter, "-s") == 0)
    {
        _scale = value?std::atof(value):0;
    }
    else if (std::strcmp(parameter, "-r") == 0)
    {
        _ratio = value?std::atof(value):0;
    }
    else
    {
        std::string message = "Invalid parameter : ";
        message += parameter;
        throw CustomException(message, CustomException::Level::NORMAL);
    }
}

void Parameters::check()
{
    if (_tilesPath == "")
    {
        throw CustomException("No path defined, use -p option", CustomException::Level::NORMAL);
    }
    else if (!std::filesystem::exists(_tilesPath))
    {
        std::string message = "Invalid path : ";
        message += _tilesPath;
        message += ", use -p option";
        throw CustomException(message, CustomException::Level::NORMAL);
    }
    if (_photoPath == "")
    {
        throw CustomException("No image defined, use -i option", CustomException::Level::NORMAL);
    }
    else if (!std::filesystem::exists(_photoPath))
    {
        std::string message = "Invalid file : ";
        message += _photoPath;
        message += ", use -i option";
        throw CustomException(message, CustomException::Level::NORMAL);
    }
    if (_subdivision <= 0)
    {
        throw CustomException("Invalid subdivision value, use -d option", CustomException::Level::NORMAL);
    }
    if (_scale <= 0)
    {
        throw CustomException("Invalid scale value, use -s option", CustomException::Level::NORMAL);
    }
    if (_ratio < 0)
    {
        throw CustomException("Invalid ratio value, use -r option", CustomException::Level::NORMAL);
    }
}
