#pragma once

#include "photo.h"
#include "tiles.h"



class MatchSolver {
public:
    MatchSolver(const Photo &photo, const Tiles &tiles, int subdivisions);
    ~MatchSolver();
    int *getMatchingTiles();

private:
    int findBestTile(const std::vector<Tiles::Data> &tileData, const double *features);
    void printInfo() const;

private:
    int *_matchingTiles;
    int _subdivisions;
};
