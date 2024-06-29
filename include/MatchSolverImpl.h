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
        double _sqDist;

        MatchCandidate() : _id(-1), _sqDist(0) {};

        bool operator<(const MatchCandidate& rhs) const
        {
            return this->_sqDist < rhs._sqDist;
        }
    };

    struct InitCandidate : public MatchCandidate
    {
        int _i;
        int _j;

        InitCandidate(int i, int j) : _i(i), _j(j) {};
    };

private:
    void computeRedundancyBox(int i, int j, cv::Rect& box) const;
    void findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const ITiles& tiles) const;
    void reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const;
    void findInitialSolution(std::vector<std::vector<MatchCandidate>>& candidates);

private:
    std::vector<int> _matchingTiles;
    double _cost;
};