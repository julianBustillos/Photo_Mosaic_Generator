#pragma once

#include <string>


class Parameters 
{
public:
	static std::string getHelp();

public:
	Parameters(int argc, char *argv[]);
	~Parameters() {};
	std::string getPhotoPath() const;
	std::string getTilesPath() const;
    double getScale() const;
    double getRatio() const;
	int getSubdivision() const;

private:
	void parse(char *parameter, char *value);
	void check();

private:
	std::string _tilesPath = "";
	std::string _photoPath = "";
    double _scale = 1.;
    double _ratio = 0.;
	int _subdivision = 0;
};