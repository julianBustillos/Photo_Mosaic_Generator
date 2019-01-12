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
	const int getSubdivision();

private:
	void parseArgument(char *parameter, char *value);
	void checkParsing();

private:
	std::string _tilesPath = "";
	std::string _photoPath = "";
	unsigned int _subdivision = 0;
};