#pragma once

#include "IMatchSolver.h"


class MatchSolverImpl : public IMatchSolver
{
public:
    MatchSolverImpl(const Photo &photo, int subdivisions) : IMatchSolver(photo, subdivisions) {}
    virtual ~MatchSolverImpl() {};
    virtual void solve(const ITiles &tiles);
    virtual const std::vector<int> &getMatchingTiles() const;

private:
    struct matchCandidate {
        int _id = -1;
        int _i = -1;
        int _j = -1;
        double _squareDistance;

        matchCandidate(int i, int j) : _i(i), _j(j) {};

        bool operator<(const matchCandidate& rhs) const
        {
            return this->_squareDistance < rhs._squareDistance;
        }
    };

private:
    void findCandidateTiles(std::vector<matchCandidate> &candidates, int i, int j, const ITiles &tiles);
    void findBestTiles(std::vector<matchCandidate> &candidates);
    void printInfo() const;

private:
    static const int _redundancyTilesNumber = (REDUNDANCY_DISTANCE * 2 + 1) * (REDUNDANCY_DISTANCE * 2 + 1);

private:
    std::vector<int> _matchingTiles;
};