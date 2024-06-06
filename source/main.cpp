#include <iostream>
#include "Clock.h"
#include "CustomException.h"
#include "Parameters.h"
#include "MosaicGenerator.h"


int main(int argc, char* argv[])
{
    Parameters parameters;

    try
    {
        std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++ PHOTO MOSAIC GENERATOR ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl << std::endl;

        Clock clock;
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
            std::cout << parameters.getHelp() << std::endl;
            break;
        case CustomException::Level::NORMAL:
            std::cout << e.what() << std::endl << std::endl;
            std::cout << parameters.getHelp() << std::endl;
            break;
        case CustomException::Level::ERROR:
            std::cerr << "ERROR : " << e.what() << std::endl;
            break;
        default:
            std::cerr << "ERROR : we should not be here, you have to DEBUG me !!" << std::endl;
        }
        exit(EXIT_SUCCESS);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR unhandled exception : " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}