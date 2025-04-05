#include "Parameters.h"
#include "CustomException.h"
#include "Log.h"
#include <filesystem>
#include <algorithm>


Parameters::Parameters() :
    _options("Photo_Mosaic_Generator.exe", "Generates an awesome mosaic to match a photo using a photo database.")
{
    _options.add_options()
        ("p,photo", "Path to reference photo for mosaic.", cxxopts::value<std::string>())
        ("t,tiles", "Path to tiles image folder.", cxxopts::value<std::string>())
        ("g,grid", "Grid size (width, height) for tiling. Could be one value (equal on both dimensions) or two values. Separator [,].", cxxopts::value<std::vector<int>>())
        ("s,scale", "Photo scale value for outputs resolution. Not compatible with resolution usage.", cxxopts::value<double>())
        ("r,resolution", "Resolution values (width, height) for outputs. Not compatible with scale usage.Separator [,].", cxxopts::value<std::vector<int>>())
        ("c,crop", "Allow cropping photo when resolution mode is enabled. Can only be used with resolution option.")
        ("b,blending", "Blending values for outputs. Could be one or three values: step for exported mosaics [0.01;1], minimum value >= 0, maximum value <= 1. Separator [,].", cxxopts::value<std::vector<double>>()->default_value("0.1"))
        ("h,help", "Print usage");
}

void Parameters::initialize(int argc, char* argv[])
{
    parse(argc, argv);
    check();

    Log::Logger::get().log(Log::TRACE) << "Parameter checked.";
    Log::Logger::get().log(Log::DEBUG) << "Photo path : " << _photoPath.value();
    Log::Logger::get().log(Log::DEBUG) << "Tiles path : " << _tilesPath.value();
    Log::Logger::get().log(Log::DEBUG) << "Grid : " << _grid.value();
    if (_scale.has_value())
        Log::Logger::get().log(Log::DEBUG) << "Scale : " << _scale.value();
    if (_resolution.has_value())
        Log::Logger::get().log(Log::DEBUG) << "Resolution : " << _resolution.value();
    Log::Logger::get().log(Log::DEBUG) << "Crop : " << (_crop ? "true" : "false");
    Log::Logger::get().log(Log::DEBUG) << "Blending : " << _blending.value();
}

std::string Parameters::getPhotoPath() const
{
    return _photoPath.value();
}

std::string Parameters::getTilesPath() const
{
    return _tilesPath.value();
}

std::tuple<int, int>  Parameters::getGrid() const
{
    return std::make_tuple(_grid.value()[0], _grid.value()[1]);
}

double Parameters::getScale() const
{
    return _scale.has_value() ? _scale.value() : 0;
}

std::tuple<int, int, bool>  Parameters::getResolution() const
{
    if (_resolution.has_value())
        return std::make_tuple(_resolution.value()[0], _resolution.value()[1], _crop);
    else
        return std::make_tuple(0, 0, false);
}

std::tuple<double, double, double>  Parameters::getBlending() const
{
    return std::make_tuple(_blending.value()[0], _blending.value()[1], _blending.value()[2]);
}

std::string Parameters::getHelp() const
{
    return "------- HELP -------\n" + _options.help();
}

void Parameters::parse(int argc, char* argv[])
{
    _options.allow_unrecognised_options();
    cxxopts::ParseResult result = _options.parse(argc, argv);

    std::string params;
    for (int i = 0; i < argc; i++)
    {
        params += argv[i];
        params += " ";
    }
    Log::Logger::get().log(Log::INFO) << "Launching call : " << params;

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
    if (result.count("grid"))
        _grid = result["grid"].as<std::vector<int>>();
    if (result.count("scale"))
        _scale = result["scale"].as<double>();
    if (result.count("resolution"))
        _resolution = result["resolution"].as<std::vector<int>>();
    if (result.count("crop"))
        _crop = true;
    _blending = result["blending"].as<std::vector<double>>();
}

void Parameters::check()
{
    std::string message = "Arguments check : ";
    unsigned int errorCount = 0;

    if (!_photoPath.has_value())
    {
        message += "\nNo photo defined";
        errorCount++;
    }
    else
    {
        std::replace(_photoPath.value().begin(), _photoPath.value().end(), '/', '\\');
        if (!std::filesystem::exists(_photoPath.value()))
        {
            message += "\nInvalid file : " + _photoPath.value();
            errorCount++;
        }
    }

    if (!_tilesPath.has_value())
    {
        message += "\nNo tiles path defined";
        errorCount++;
    }
    else 
    {
        std::replace(_tilesPath.value().begin(), _tilesPath.value().end(), '/', '\\');
        if (_tilesPath.value().back() != '\\')
            _tilesPath.value() += "\\";

        if (!std::filesystem::exists(_tilesPath.value()))
        {
            message += "\nInvalid path : " + _tilesPath.value();
            errorCount++;
        }
    }

    if (!_grid.has_value())
    {
        message += "\nNo grid values defined";
        errorCount++;
    }
    else if (_grid.value().empty() || _grid.value().size() > 2)
    {
        message += "\nWrong number of grid elements : " + std::to_string(_grid.value().size());
        errorCount++;
    }
    else
    {
        for (int i = 0; i < _grid.value().size(); i++)
        {
            if (_grid.value()[i] <= 0)
            {
                message += "\nInvalid grid value : " + std::to_string(_grid.value()[i]);
                errorCount++;
            }
        }
        if (_grid.value().size() == 1)
            _grid.value().push_back(_grid.value()[0]);
    }

    if (_scale.has_value() && _resolution.has_value())
    {
        message += "\nInvalid use of scale and resolution exclusive options";
        errorCount++;
    }
    else
    {
        if (_scale.has_value() && _scale.value() <= 0)
        {
            message += "\nInvalid scale value : " + std::to_string(_scale.value());
            errorCount++;
        }
        if (_resolution.has_value())
        {
            if (_resolution.value().empty())
            {
                message += "\nNo resolution defined";
                errorCount++;
            }
            else if (_resolution.value().size() != 2)
            {
                message += "\nWrong number of resolution elements : " + std::to_string(_resolution.value().size());
                errorCount++;
            }
            else
            {
                for (int i = 0; i < 2; i++)
                {
                    if (_resolution.value()[i] <= 0)
                    {
                        message += "\nInvalid resolution value : " + std::to_string(_resolution.value()[i]);
                        errorCount++;
                    }
                }
            }
        }
    }
   
    if (_crop && !_resolution.has_value())
    {
        message += "\nCrop option can only be used if resolution is enabled";
        errorCount++;
    }

    if (_blending.has_value())
    {
        if (_blending.value().size() != 1 && _blending.value().size() != 3)
        {
            message += "\nWrong number of blending elements : " + std::to_string(_blending.value().size());
            errorCount++;
        }
        else
        {
            if (_blending.value()[0] < 0.01 || 1. < _blending.value()[0])
            {
                message += "\nInvalid ratio value";
                errorCount++;
            }
            if (_blending.value().size() == 3)
            {
                if (_blending.value()[1] < 0)
                {
                    message += "\nInvalid blending minimum value : " + std::to_string(_blending.value()[1]);
                    errorCount++;
                }
                if (_blending.value()[2] > 1)
                {
                    message += "\nInvalid blending maximum value : " + std::to_string(_blending.value()[2]);
                    errorCount++;
                }
                if (_blending.value()[1] > _blending.value()[2])
                {
                    message += "\nMinimum value > Maximum value : " + std::to_string(_blending.value()[1]) + " > " + std::to_string(_blending.value()[2]);
                    errorCount++;
                }
            }
            else
            {
                _blending.value().push_back(0);
                _blending.value().push_back(1);
            }
        }
    }

    if (errorCount > 0)
    {
        throw CustomException(message, CustomException::Level::NORMAL);
    }
}

