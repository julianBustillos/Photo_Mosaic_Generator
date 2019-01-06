#pragma once

#include <string>


class Parameters {
public:
	static std::string getHelp();

public:
	Parameters() {};
	~Parameters() {};
	void parse(int argc, char *argv[]);

private:
	void getArgument(char *parameter, char *value);
	void checkParsing();
	bool pathExist(std::string &path);

private:
	std::string imageDirectoryPath = "";
	std::string mainImagePath = "";
	unsigned int subdivision = 0;
};