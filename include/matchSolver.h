#pragma once

#include "photo.h"
#include "tiles.h"



class MatchSolver {
public:
    MatchSolver(const Photo &photo, const Tiles &tiles, int subdivisions);
    ~MatchSolver();
    int *getMatchingTiles();

private:
    struct matchCandidate {
        int _id;
        int _i;
        int _j;
        double _squareDistance;

        bool operator<(const matchCandidate& rhs) const
        {
            return this->_squareDistance < rhs._squareDistance;
        }
    };

private:
    //int findBestTile(const std::vector<Tiles::Data> &tileData, const double *features);
    void findCandidateTiles(std::vector<matchCandidate> &candidates, int i, int j, const std::vector<Tiles::Data> &tileData, const double *features);
    void findBestTiles(std::vector<matchCandidate> &candidates);
    void printInfo() const;

private:
    static const int _redundancyTilesNumber = (REDUNDANCY_DISTANCE * 2 + 1) * (REDUNDANCY_DISTANCE * 2 + 1);

private:
    int *_matchingTiles;
    int _subdivisions;
};
