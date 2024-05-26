#include "Parameters.h"
#include "CustomException.h"
#include <filesystem>
#include <algorithm>


Parameters::Parameters() :
    _options("Photo_Mosaic_Generator.exe", "Generates an awesome mosaic to match a photo using a photo database.")
{
    _options.add_options()
        ("p,photo", "Path to reference photo for mosaic", cxxopts::value<std::string>())
        ("t,tiles", "Path to tiles image folder", cxxopts::value<std::string>())
        ("d,subdiv", "Image subdivision (height and width)", cxxopts::value<int>())
        ("s,scale", "Image scale", cxxopts::value<double>()->default_value("1."))
        ("r,ratio", "Image ratio (width / height)", cxxopts::value<double>()->default_value("0."))
        ("h,help", "Print usage");
}

void Parameters::initialize(int argc, char* argv[])
{
    parse(argc, argv);
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

std::string Parameters::getHelp() const
{
    return "------- HELP -------\n" + _options.help();
}

void Parameters::parse(int argc, char* argv[])
{
    _options.allow_unrecognised_options();
    cxxopts::ParseResult result = _options.parse(argc, argv);

    if (result.count("help"))
    {
        throw CustomException("", CustomException::Level::HELP);
    }

    cxxopts::OptionNames unmatched = result.unmatched();
    if (!unmatched.empty())
    {
        std::string message = "Invalid parameters :";
        for (int p = 0; p < unmatched.size(); p++)
        {
            message += " " + unmatched[p];
        }
        throw CustomException(message, CustomException::Level::NORMAL);
    }

    if (result.count("photo"))
        _photoPath = result["photo"].as<std::string>();
    if (result.count("tiles"))
        _tilesPath = result["tiles"].as<std::string>();
    if (result.count("subdiv"))
        _subdivision = result["subdiv"].as<int>();
    _scale = result["scale"].as<double>();
    _ratio = result["ratio"].as<double>();

    std::replace(_photoPath.begin(), _photoPath.end(), '/', '\\');
    std::replace(_tilesPath.begin(), _tilesPath.end(), '/', '\\');
    if (_tilesPath.back() != '\\')
        _tilesPath += "\\";
}

void Parameters::check()
{
    std::string message = "ARGUMENTS : ";
    unsigned int errorCount = 0;

    if (_photoPath == "")
    {
        message += "\nNo photo defined";
        errorCount++;
    }
    else if (!std::filesystem::exists(_photoPath))
    {
        message += "\nInvalid file : " + _photoPath;
        errorCount++;
    }
    else if (_tilesPath == "")
    {
        message += "\nNo tiles path defined";
        errorCount++;
    }
    else if (!std::filesystem::exists(_tilesPath))
    {
        message += "\nInvalid path : " + _tilesPath;
        errorCount++;
    }
    else if (_subdivision <= 0)
    {
        message += "\nInvalid subdivision value";
        errorCount++;
    }
    else if (_scale <= 0)
    {
        message += "\nInvalid scale value";
        errorCount++;
    }
    else if (_ratio < 0)
    {
        message += "\nInvalid ratio value";
        errorCount++;
    }

    if (errorCount > 0)
    {
        throw CustomException(message, CustomException::Level::NORMAL);
    }
}
