#include "MatchSolverImpl.h"
#include <opencv2/opencv.hpp>
#include "ITiles.h"
#include <vector>
#include <algorithm>
#include "Log.h"
#include "Console.h"
#include "CustomException.h"


MatchSolverImpl::MatchSolverImpl(int subdivisions) : 
    IMatchSolver(subdivisions)
{
}

MatchSolverImpl::~MatchSolverImpl()
{
}

int MatchSolverImpl::getRequiredNbTiles()
{
    return RedundancyTilesNumber;
}

void MatchSolverImpl::solve(const ITiles& tiles)
{
    Console::Out::get(Console::DEFAULT) << "Computing tiles matching...";
    _matchingTiles.resize(_subdivisions * _subdivisions, -1);

    std::vector<matchCandidate> candidates;

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            findCandidateTiles(candidates, i, j, tiles);
        }
    }
    Log::Logger::get().log(Log::TRACE) << "Candidate tiles found.";

    findBestTiles(candidates);
}

const std::vector<int>& MatchSolverImpl::getMatchingTiles() const
{
    return _matchingTiles;
}

void MatchSolverImpl::findCandidateTiles(std::vector<matchCandidate>& candidates, int i, int j, const ITiles& tiles)
{
    std::vector<matchCandidate> tileCandidates(tiles.getNbTiles());
    matchCandidate candidate(i, j);

    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        candidate._id = t;
        candidate._squareDistance = tiles.computeSquareDistance(i, j, t);
        tileCandidates[t] = candidate;
    }

    std::sort(tileCandidates.begin(), tileCandidates.end());
    for (int k = 0; k < RedundancyTilesNumber; k++)
        candidates.emplace_back(tileCandidates[k]);
}

void MatchSolverImpl::findBestTiles(std::vector<matchCandidate>& candidates)
{
    std::sort(candidates.begin(), candidates.end());
    double distance = 0;
    for (int k = 0; k < candidates.size(); k++)
    {
        int candidateId = candidates[k]._i * _subdivisions + candidates[k]._j;
        if (_matchingTiles[candidateId] >= 0)
            continue;

        int iMin = std::max(candidates[k]._i - RedundancyDistance, 0);
        int iMax = std::min(candidates[k]._i + RedundancyDistance, _subdivisions - 1);
        int jMin = std::max(candidates[k]._j - RedundancyDistance, 0);
        int jMax = std::min(candidates[k]._j + RedundancyDistance, _subdivisions - 1);

        bool redundancy = false;
        for (int i = iMin; i <= iMax && !redundancy; i++)
        {
            for (int j = jMin; j <= jMax && !redundancy; j++)
            {
                int currentId = i * _subdivisions + j;
                if (_matchingTiles[currentId] == candidates[k]._id)
                    redundancy = true;
            }
        }

        if (redundancy)
            continue;

        _matchingTiles[candidateId] = candidates[k]._id;
        distance += sqrt(candidates[k]._squareDistance);

    }
    Log::Logger::get().log(Log::TRACE) << "Matching tiles found.";
    Log::Logger::get().log(Log::TRACE) << "With mean square distance : " << (distance / (_subdivisions * _subdivisions));
}

