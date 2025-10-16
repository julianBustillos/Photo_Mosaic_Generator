#pragma once

#include "Tiles.h"
#include <tuple>
#include <vector>
#include <opencv2/opencv.hpp>


class MatchSolver
{
private:
    static constexpr int RedundancyRadius = 5;
    static constexpr int MaskSize = 2 * RedundancyRadius - 1;

public:
    MatchSolver(std::tuple<int, int> grid);
    ~MatchSolver();

public:
    int getRequiredNbTiles();
    void solve(const Tiles& tiles);
    const std::vector<int>& getUniqueIds() const;
    int getMatchingId(int mosaicId) const;

private:
    struct MatchCandidate //TODO improve structure
    {
        int _id;
        double _dist;

        MatchCandidate() : _id(-1), _dist(0) {};

        bool operator<(const MatchCandidate& rhs) const
        {
            return _dist < rhs._dist;
        }
    };

    struct SortCandidate : public MatchCandidate
    {
        int _i;
        int _j;

        SortCandidate(int i, int j) : _i(i), _j(j) {};
    };

private:
    void computeMaskLimits(int i, int j, int& maskStart, int& maskStep, int& iMaskSize, int& jMaskSize, int& gridStart, int& gridStep) const;
    void findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const Tiles& tiles) const;
    void reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const;
    void findSolution(std::vector<std::vector<MatchCandidate>>& candidates);

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<bool> _redundancyMask;
    int _redundancyMaskNbTiles;
    std::vector<int> _uniqueIds;
    std::vector<int> _matchingIds;
    double _matchingCost;
};