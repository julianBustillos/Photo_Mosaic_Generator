#pragma once

#include <string>
#include "cxxopts.h"


class Parameters 
{
public:
	Parameters();
	~Parameters() {};
	void initialize(int argc, char* argv[]);
	std::string getPhotoPath() const;
	std::string getTilesPath() const;
	int getSubdivision() const;
	double getScale() const;
    double getRatio() const;
    double getBlending() const;
    double getBlendingMin() const;
    double getBlendingMax() const;
	std::string getHelp() const;

private:
	void parse(int argc, char* argv[]);
	void check();

private:
	cxxopts::Options _options;
	std::string _tilesPath = "";
	std::string _photoPath = "";
	int _subdivision = 0;
	double _scale = 1.;
    double _ratio = 0.;
	double _blending = 0.1;
	double _blendingMin = 0;
	double _blendingMax = 1;
};