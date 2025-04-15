#pragma once

#include "Tiles.h"
#include <tuple>
#include <opencv2/opencv.hpp>


class MatchSolver
{
private:
    static constexpr int RedundancyDistance = 4;
    static constexpr int RedundancyTilesNumber = (RedundancyDistance * 2 + 1) * (RedundancyDistance * 2 + 1);

public:
    MatchSolver(std::tuple<int, int> grid);
    ~MatchSolver();

public:
    int getRequiredNbTiles();
    void solve(const Tiles& tiles);
    int getMatchingTile(int mosaicId) const;

private:
    struct MatchCandidate
    {
        int _id;
        double _dist;

        MatchCandidate() : _id(-1), _dist(0) {};

        bool operator<(const MatchCandidate& rhs) const
        {
            return _dist < rhs._dist;
        }
    };

    struct InitCandidate : public MatchCandidate
    {
        int _i;
        int _j;

        InitCandidate(int i, int j) : _i(i), _j(j) {};
    };

    struct SearchNode
    {
        int _depth;
        int _tile;
        double _cost;

        SearchNode(int depth, int tile, double cost) : _depth(depth), _tile(tile), _cost(cost) {};
    };

private:
    void computeRedundancyBox(int i, int j, cv::Rect& box) const;
    void findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const Tiles& tiles) const;
    void reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const;
    void findInitialSolution(std::vector<std::vector<MatchCandidate>>& candidates);

private:
    const int _gridWidth;
    const int _gridHeight;
    std::vector<int> _bestSolution;
    double _bestCost;
};