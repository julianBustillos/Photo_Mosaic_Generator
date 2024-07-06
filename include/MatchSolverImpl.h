#pragma once

#include "IMatchSolver.h"
#include <opencv2/opencv.hpp>


class MatchSolverImpl : public IMatchSolver
{
private:
    static constexpr int RedundancyDistance = 4; //TODO OPTION ??
    static constexpr int RedundancyTilesNumber = (RedundancyDistance * 2 + 1) * (RedundancyDistance * 2 + 1);

public:
    MatchSolverImpl(int subdivisions);
    virtual ~MatchSolverImpl();

public:
    virtual int getRequiredNbTiles();
    virtual void solve(const ITiles& tiles);
    virtual const std::vector<int>& getMatchingTiles() const;

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
    void findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const ITiles& tiles) const;
    void reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const;
    void findInitialSolution(std::vector<std::vector<MatchCandidate>>& candidates);

private:
    std::vector<int> _bestSolution;
    double _bestCost;
};