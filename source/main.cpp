#include <iostream>
#include <fstream>
#include <opencv2/core/utils/logger.hpp>
#include "termcolor.h"
#include "Clock.h"
#include "CustomException.h"
#include "Parameters.h"
#include "MosaicGenerator.h"
#include "SystemUtils.h"
#include "Log.h"
#include "Console.h"


int main(int argc, char* argv[])
{
    int exitCode = EXIT_SUCCESS;

    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

    Parameters parameters;

    try
    {
        Clock clock;
        //TODO MOVE COUT
        Console::Out::get(Console::DEFAULT) << "++++++++++++++++++++++++++++++++++++++++++++++++ PHOTO MOSAIC GENERATOR ++++++++++++++++++++++++++++++++++++++++++++++++";
        Console::Out::get(Console::DEFAULT) << "";

        const std::string logPath = SystemUtils::getCurrentProcessDirectory() + "/log.txt";
        Log::Logger::get().setStream(new std::ofstream(logPath), true);
#ifdef NDEBUG
        Log::Logger::get().setLevel(Log::Level::INFO);
#else
        Log::Logger::get().setLevel(Log::Level::TRACE);
#endif

        parameters.initialize(argc, argv);
        MosaicGenerator generator(parameters);
        generator.Build();

        std::string timeStamp = clock.getTimeStamp();
        Console::Out::get(Console::TIME) << timeStamp;
        Log::Logger::get().log(Log::TRACE) << timeStamp;

        Console::Out::get(Console::DEFAULT) << "";
        Console::Out::get(Console::DEFAULT) << "++++++++++++++++++++++++++++++++++++++++++++++++           END          ++++++++++++++++++++++++++++++++++++++++++++++++";
    }
    catch (CustomException& e)
    {
        switch (e.getLevel())
        {
        case CustomException::Level::HELP:
            Console::Out::get(Console::HELP) << parameters.getHelp();
            break;
        case CustomException::Level::NORMAL:
            Console::Out::get(Console::HELP) << e.what();
            Console::Out::get(Console::HELP) << parameters.getHelp();
            Log::Logger::get().log(Log::WARN) << e.what();
            break;
        case CustomException::Level::ERROR:
            Console::Out::get(Console::ERROR) << e.what();
            Log::Logger::get().log(Log::ERROR) << e.what();
            break;
        default:
            std::string message = "Unknown CustomException !!";
            Console::Out::get(Console::ERROR) << message;
            Log::Logger::get().log(Log::FATAL) << message;
        }
    }
    catch (std::exception& e)
    {
        std::string message = "Unhandled exception : ";
        Console::Out::get(Console::ERROR) << e.what();
        Log::Logger::get().log(Log::FATAL) << message << e.what();
        exitCode = EXIT_FAILURE;
    }

    return exitCode;
}