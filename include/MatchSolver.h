#pragma once

#include "Tiles.h"
#include <tuple>
#include <vector>
#include <opencv2/opencv.hpp>


class MatchSolver
{
private:
    static constexpr int RedundancyDist = 4;
    static constexpr int RedundancyNbTiles = (RedundancyDist * 2 + 1) * (RedundancyDist * 2 + 1);

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
    void computeRedundancyBox(int i, int j, cv::Rect& box) const;
    void findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const Tiles& tiles) const;
    void reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const;
    void findSolution(std::vector<std::vector<MatchCandidate>>& candidates);

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<int> _uniqueIds;
    std::vector<int> _matchingIds;
    double _matchingCost;
};