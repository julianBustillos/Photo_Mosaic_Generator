#include <iostream>
#include <fstream>
#include <opencv2/core/utils/logger.hpp>
#define NOMINMAX
#include "termcolor.h"
#include "Clock.h"
#include "CustomException.h"
#include "Parameters.h"
#include "MosaicGenerator.h"
#include "SystemUtils.h"
#include "Log.h"


int main(int argc, char* argv[])
{
    int exitCode = EXIT_SUCCESS;

    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);
    std::cout << termcolor::bright_green;

    Parameters parameters;

    try
    {
        Clock clock;
        //TODO MOVE COUT
        std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++ PHOTO MOSAIC GENERATOR ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl << std::endl;

        const std::string logPath = SystemUtils::getCurrentProcessDirectory() + "/log.txt";
        Log::Logger::getInstance().setStream(new std::ofstream(logPath), true);
#ifdef NDEBUG
        Log::Logger::getInstance().setLevel(Log::Level::INFO);
#else
        Log::Logger::getInstance().setLevel(Log::Level::TRACE);
#endif

        parameters.initialize(argc, argv);
        MosaicGenerator generator(parameters);
        generator.Build();

        std::string timeStamp = clock.getTimeStamp();
        std::cout << termcolor::bright_blue;
        std::cout << timeStamp << std::endl;
        Log::Logger::getInstance().log(Log::TRACE) << timeStamp;

        std::cout << std::endl << "++++++++++++++++++++++++++++++++++++++++++++++++           END          ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    }
    catch (CustomException& e)
    {
        switch (e.getLevel())
        {
        case CustomException::Level::HELP:
            std::cout << termcolor::bright_yellow;
            std::cout << parameters.getHelp() << std::endl;
            break;
        case CustomException::Level::NORMAL:
            std::cout << termcolor::bright_yellow;
            std::cout << e.what() << std::endl << std::endl;
            Log::Logger::getInstance().log(Log::WARN) << e.what();
            std::cout << parameters.getHelp() << std::endl;
            break;
        case CustomException::Level::ERROR:
            std::cerr << termcolor::bright_red;
            std::cerr << e.what() << std::endl;
            Log::Logger::getInstance().log(Log::ERROR) << e.what();
            break;
        default:
            std::cout << termcolor::bright_red;
            std::string message = "Unknown CustomException !!";
            std::cerr << message << std::endl;
            Log::Logger::getInstance().log(Log::FATAL) << message;
        }
    }
    catch (std::exception& e)
    {
        std::cout << termcolor::bright_red;
        std::string message = "Unhandled exception : ";
        std::cerr << message << e.what() << std::endl;
        Log::Logger::getInstance().log(Log::FATAL) << message << e.what();
        exitCode = EXIT_FAILURE;
    }

    std::cout << termcolor::reset;
    std::cerr << termcolor::reset;
    return exitCode;
}