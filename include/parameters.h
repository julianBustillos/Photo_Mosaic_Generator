#pragma once

#include <string>


class Parameters {
public:
	static std::string getHelp();

public:
	Parameters(int argc, char *argv[]);
	~Parameters() {};
	const std::string getPhotoPath();
	const std::string getTilesPath();
    const int getWidth();
    const int getHeight();
	const int getSubdivision();

private:
	void parseArgument(char *parameter, char *value);
	void checkParsing();

private:
	std::string _tilesPath = "";
	std::string _photoPath = "";
    int _width = 0;
    int _height = 0;
	int _subdivision = 0;
};