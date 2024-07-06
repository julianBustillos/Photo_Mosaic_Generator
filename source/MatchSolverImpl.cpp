#include "MatchSolverImpl.h"
#include <opencv2/opencv.hpp>
#include "ITiles.h"
#include <vector>
#include <algorithm>
#include <stack>
#include "Log.h"
#include "Console.h"
#include "CustomException.h"


MatchSolverImpl::MatchSolverImpl(int subdivisions) : 
    IMatchSolver(subdivisions), _bestCost(-1)
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
    const int mosaicSize = _subdivisions * _subdivisions;
    _bestSolution.resize(mosaicSize, -1);
    std::vector<std::vector<MatchCandidate>> candidates(mosaicSize);

    findCandidateTiles(candidates, tiles);
    reduceCandidateTiles(candidates);
    findInitialSolution(candidates);
    Log::Logger::get().log(Log::TRACE) << "Best tiles found.";
}

const std::vector<int>& MatchSolverImpl::getMatchingTiles() const
{
    return _bestSolution;
}

void MatchSolverImpl::computeRedundancyBox(int i, int j, cv::Rect& box) const
{
    box.y = std::max(i - RedundancyDistance, 0);
    box.height = std::min(i + RedundancyDistance, _subdivisions - 1) - box.y + 1;
    box.x = std::max(j - RedundancyDistance, 0);
    box.width = std::min(j + RedundancyDistance, _subdivisions - 1) - box.x + 1;
}

void MatchSolverImpl::findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const ITiles& tiles) const
{
    int m = 0;
    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++, m++)
        {
            candidates[m].resize(tiles.getNbTiles());

            for (int t = 0; t < tiles.getNbTiles(); t++)
            {
                candidates[m][t]._id = t;
                candidates[m][t]._dist = tiles.computeDistance(i, j, t);
            }

            std::sort(candidates[m].begin(), candidates[m].end());
            candidates[m].resize(RedundancyTilesNumber);
        }
    }
}

void MatchSolverImpl::reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const
{
    std::vector<std::vector<int>> sortedId(candidates.size());
    for (int m = 0; m < candidates.size(); m++)
    {
        sortedId[m].resize(candidates[m].size());
        for (int t = 0; t < candidates[m].size(); t++)
        {
            sortedId[m][t] = candidates[m][t]._id;
        }
        std::sort(sortedId[m].begin(), sortedId[m].end());
    }

    int lastReduction = -1;
    cv::Rect box;
    while (true)
    {
        int m = 0;
        for (int i = 0; i < _subdivisions; i++)
        {
            for (int j = 0; j < _subdivisions; j++, m++)
            {
                if (lastReduction == m)
                    return;

                if (candidates[m].size() < 2)
                    continue;

                computeRedundancyBox(i, j, box);
                for (int t = 0; t < candidates[m].size() - 1; t++)
                {
                    bool redundancy = false;
                    int currId = box.y * _subdivisions + box.x;
                    const int step = _subdivisions - box.width;
                    for (int iBox = 0; iBox < box.height && !redundancy; iBox++, currId += step)
                    {
                        for (int jBox = 0; jBox < box.width && !redundancy; jBox++, currId++)
                        {
                            if (std::binary_search(sortedId[currId].begin(), sortedId[currId].end(), candidates[m][t]._id))
                                redundancy = true;
                        }
                    }

                    if (!redundancy)
                    {
                        candidates[m].resize(t + 1);
                        sortedId[m].resize(t + 1);
                        for (int t = 0; t < candidates[m].size(); t++)
                        {
                            sortedId[m][t] = candidates[m][t]._id;
                        }
                        std::sort(sortedId[m].begin(), sortedId[m].end());
                        lastReduction = m;
                    }
                }
            }
        }

        if (lastReduction < 0)
            return;
    }
}

void MatchSolverImpl::findInitialSolution(std::vector<std::vector<MatchCandidate>>& candidates)
{
    std::vector<InitCandidate> initCandidates;
    for (int m = 0; m < candidates.size(); m++)
    {
        const int i = m / _subdivisions;
        const int j = m - i * _subdivisions;
        const int start = initCandidates.size();
        initCandidates.resize(start + candidates[m].size(), InitCandidate(i, j));
        for (int t = 0; t < candidates[m].size(); t++)
        {
            initCandidates[start + t]._id = candidates[m][t]._id;
            initCandidates[start + t]._dist = candidates[m][t]._dist;
        }
    }
    std::sort(initCandidates.begin(), initCandidates.end());
   
    _bestCost = 0;
    cv::Rect box;
    for (int k = 0; k < initCandidates.size(); k++)
    {
        int candidateId = initCandidates[k]._i * _subdivisions + initCandidates[k]._j;
        if (_bestSolution[candidateId] >= 0)
            continue;

        computeRedundancyBox(initCandidates[k]._i, initCandidates[k]._j, box);
        bool redundancy = false;
        int currId = box.y * _subdivisions + box.x;
        const int step = _subdivisions - box.width;
        for (int i = 0; i < box.height && !redundancy; i++, currId += step)
        {
            for (int j = 0; j < box.width && !redundancy; j++, currId++)
            {
                if (_bestSolution[currId] == initCandidates[k]._id)
                    redundancy = true;
            }
        }

        if (redundancy)
            continue;

        _bestSolution[candidateId] = initCandidates[k]._id;
        _bestCost += initCandidates[k]._dist;

    }
    Log::Logger::get().log(Log::TRACE) << "Matching tiles initial solution found.";
    Log::Logger::get().log(Log::TRACE) << "With mean cost : " << (_bestCost / (_subdivisions * _subdivisions));
}

