#pragma once

#include <string>
#include <optional>
#include <tuple>
#include "cxxopts.hpp"


class Parameters
{
public:
	Parameters();
	~Parameters() {};
	void initialize(int argc, char* argv[]);
	std::string getPhotoPath() const;
	std::string getTilesPath() const;
	std::tuple<int, int> getGrid() const;
	double getScale() const;
	std::tuple<int, int, bool> getResolution() const;
	std::tuple<double, double, double> getBlending() const;
	std::string getHelp() const;

private:
	void parse(int argc, char* argv[]);
	void check();

private:
	cxxopts::Options _options;
	std::optional<std::string> _tilesPath;
	std::optional<std::string> _photoPath;
	std::optional<std::vector<int>> _grid;
	std::optional<double> _scale;
	std::optional<std::vector<int>> _resolution;
	bool _crop = false;
	std::optional<std::vector<double>> _blending;
};