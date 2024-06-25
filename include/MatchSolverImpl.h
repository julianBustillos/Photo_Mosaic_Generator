#pragma once

#include "IMatchSolver.h"


class MatchSolverImpl : public IMatchSolver
{
private:
    static constexpr int RedundancyDistance = 4; //TODO OPTION ??
    static constexpr int RedundancyTilesNumber = (RedundancyDistance * 2 + 1) * (RedundancyDistance * 2 + 1);

public:
    MatchSolverImpl(int subdivisions);
    virtual ~MatchSolverImpl();

public:
    virtual void solve(const ITiles& tiles);
    virtual const std::vector<int>& getMatchingTiles() const;

private:
    struct matchCandidate
    {
        int _id;
        int _i;
        int _j;
        double _squareDistance;

        matchCandidate(int i = -1, int j = -1) : _i(i), _j(j), _id(-1), _squareDistance(0) {};

        bool operator<(const matchCandidate& rhs) const
        {
            return this->_squareDistance < rhs._squareDistance;
        }
    };

private:
    void findCandidateTiles(std::vector<matchCandidate>& candidates, int i, int j, const ITiles& tiles);
    void findBestTiles(std::vector<matchCandidate>& candidates);

private:
    std::vector<int> _matchingTiles;
};