#pragma once

#include <vector>
#include <string>


class Tiles {
public:
	Tiles(const std::string &path);
	~Tiles() {};
	
private:
	void printInfo();

private:
	struct Data {
		std::string filename;
	};

	std::vector<Data> _tilesData;
};
