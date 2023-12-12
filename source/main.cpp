#include <iostream>
#include "clock.h"
#include "CustomException.h"
#include "Parameters.h"
#include "MosaicGenerator.h"


int main(int argc, char* argv[])
{
    try
    {
        std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++ PHOTO MOSAIC GENERATOR ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl << std::endl;

        TIME_NOW(start);

        Parameters parameters(argc, argv);
        MosaicGenerator generator(parameters);

        generator.Build();

        TIME_NOW(end);
        PRINT_DURATION(start, end);

        std::cout << std::endl << "++++++++++++++++++++++++++++++++++++++++++++++++           END          ++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
        std::cin.get();
    }
    catch (CustomException& e)
    {
        switch (e.getLevel())
        {
        case CustomException::Level::HELP:
            std::cout << Parameters::getHelp() << std::endl;
            break;
        case CustomException::Level::NORMAL:
            std::cout << e.what() << std::endl << std::endl;
            std::cout << Parameters::getHelp() << std::endl;
            break;
        case CustomException::Level::ERROR:
            std::cerr << "ERROR : " << e.what() << std::endl;
            break;
        default:
            std::cerr << "ERROR : we should not be here, you have to debug me !!" << std::endl;
        }
        std::cin.get();
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR unhandled exception : " << e.what() << std::endl;
        std::cin.get();
    }
}