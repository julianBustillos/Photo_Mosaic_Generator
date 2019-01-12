#pragma once

#include <string>


class Parameters {
public:
	static std::string getHelp();

public:
	Parameters(int argc, char *argv[]);
	~Parameters() {};
	const std::string getPhotoPath();
	const std::string tilesDirectoryPath();
	const int getSubdivision();

private:
	void parseArgument(char *parameter, char *value);
	void checkParsing();
	bool pathExist(std::string &path);

private:
	std::string _tilesDirectoryPath = "";
	std::string _photoPath = "";
	unsigned int _subdivision = 0;
};