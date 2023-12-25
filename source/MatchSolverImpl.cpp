#include "MatchSolverImpl.h"
#include "variables.h"
#include <opencv2/opencv.hpp>
#include "Utils.h"
#include "ITiles.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include "SortedVector.h"


void MatchSolverImpl::solve(const ITiles& tiles)
{
    _matchingTiles.resize(_subdivisions * _subdivisions, -1);

    std::vector<matchCandidate> candidates;

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            findCandidateTiles(candidates, i, j, tiles);
        }
    }

    findBestTiles(candidates);

    printInfo();
}

const std::vector<int>& MatchSolverImpl::getMatchingTiles() const
{
    return _matchingTiles;
}

void MatchSolverImpl::findCandidateTiles(std::vector<matchCandidate>& candidates, int i, int j, const ITiles& tiles)
{
    SortedVector<matchCandidate> tileCandidates(_redundancyTilesNumber);
    matchCandidate temp(i, j);
    std::vector<double> squareDistances;

    tiles.computeSquareDistanceVector(squareDistances, *_photo, i, j);

    for (unsigned int t = 0; t < squareDistances.size(); t++)
    {
        temp._id = t;
        temp._squareDistance = squareDistances[t];
        tileCandidates.push_sorted(temp);
    }

    for (int k = 0; k < tileCandidates.size(); k++)
        candidates.push_back(tileCandidates[k]);
}

void MatchSolverImpl::findBestTiles(std::vector<matchCandidate>& candidates)
{
    std::sort(candidates.begin(), candidates.end());
    for (int k = 0; k < candidates.size(); k++)
    {
        int candidateId = candidates[k]._i * _subdivisions + candidates[k]._j;
        if (_matchingTiles[candidateId] >= 0)
            continue;

        int iMin = std::max(candidates[k]._i - REDUNDANCY_DISTANCE, 0);
        int iMax = std::min(candidates[k]._i + REDUNDANCY_DISTANCE, _subdivisions - 1);
        int jMin = std::max(candidates[k]._j - REDUNDANCY_DISTANCE, 0);
        int jMax = std::min(candidates[k]._j + REDUNDANCY_DISTANCE, _subdivisions - 1);

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
    }
}

void MatchSolverImpl::printInfo() const
{
    std::cout << "MATCH SOLVER :" << std::endl;
    std::cout << "Number of tiles to find : " << _subdivisions * _subdivisions << std::endl;
    std::cout << "TODO (distance ?) !!" << std::endl;
    std::cout << std::endl;
}
