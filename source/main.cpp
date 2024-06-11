#include <iostream>
#include <opencv2/core/utils/logger.hpp>
#define NOMINMAX
#include "termcolor.h"
#include "Clock.h"
#include "CustomException.h"
#include "Parameters.h"
#include "MosaicGenerator.h"


int main(int argc, char* argv[])
{
    int exitCode = EXIT_SUCCESS;

    std::cout << termcolor::bright_green;
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

    Parameters parameters;
    Clock clock;

    try
    {
        std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++ PHOTO MOSAIC GENERATOR ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl << std::endl;

        clock.start();

        parameters.initialize(argc, argv);
        MosaicGenerator generator(parameters);
        generator.Build();

        clock.end();

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
            std::cout << parameters.getHelp() << std::endl;
            break;
        case CustomException::Level::ERROR:
            std::cout << termcolor::bright_red;
            std::cerr << "ERROR : " << e.what() << std::endl;
            break;
        default:
            std::cout << termcolor::bright_red;
            std::cerr << "ERROR : we should not be here, you have to DEBUG me !!" << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cout << termcolor::bright_red;
        std::cerr << "ERROR unhandled exception : " << e.what() << std::endl;
        exitCode = EXIT_FAILURE;
    }

    std::cout << termcolor::reset;
    return exitCode;
}