#include <iostream>
#include "customException.h"
#include "parameters.h"
#include "photo.h"
#include "tiles.h"


int main(int argc, char *argv[])
{
	try {
		Parameters parameters(argc, argv);
		Photo photo(parameters.getPhotoPath(), parameters.getSubdivision());
		Tiles tiles(parameters.getTilesPath());

		std::cin.get(); //DEBUG
	}
	catch (CustomException& e) {
		switch (e.getLevel()) {
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
		std::cin.get(); //DEBUG
	}
	catch (std::exception& e) {
		std::cerr << "ERROR unhandled exception : " << e.what() << std::endl;
		std::cin.get(); //DEBUG
	}
}