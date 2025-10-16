#include "MatchSolver.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <stack>
#include <set>
#include "Log.h"
#include "Console.h"
#include "CustomException.h"


MatchSolver::MatchSolver(std::tuple<int, int> grid) :
    _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid)), _matchingCost(-1)
{
    _redundancyMask.resize(MaskSize * MaskSize);
    _redundancyMaskNbTiles = 0;

    const double maxSqDist = (double)(RedundancyRadius - 0.5) * (double)(RedundancyRadius - 0.5);
    const double center = (double)(RedundancyRadius - 1);
    for (int y = 0, m = 0; y < MaskSize; y++)
    {
        double yDiff = (double)y - center;
        for (int x = 0; x < MaskSize; x++, m++)
        {
            double xDiff = (double)x - center;
            double sqDist = xDiff * xDiff + yDiff * yDiff;
            _redundancyMask[m] = (0 < sqDist && sqDist <= maxSqDist);
            if (_redundancyMask[m])
                _redundancyMaskNbTiles++;
        }
    }
}

MatchSolver::~MatchSolver()
{
}

int MatchSolver::getRequiredNbTiles()
{
    return _redundancyMaskNbTiles;
}

void MatchSolver::solve(const Tiles& tiles)
{
    Console::Out::get(Console::DEFAULT) << "Computing tiles matching...";
    const int mosaicSize = _gridWidth * _gridHeight;
    _matchingIds.resize(mosaicSize, -1);
    std::vector<std::vector<MatchCandidate>> candidates(mosaicSize);

    findCandidateTiles(candidates, tiles);
    //reduceCandidateTiles(candidates);
    findSolution(candidates);
    Log::Logger::get().log(Log::TRACE) << "Best tiles found.";
}

const std::vector<int>& MatchSolver::getUniqueIds() const
{
    return _uniqueIds;
}

int MatchSolver::getMatchingId(int mosaicId) const
{
    return _matchingIds[mosaicId];
}

void MatchSolver::computeMaskLimits(int i, int j, int& maskStart, int& maskStep, int& iMaskSize, int& jMaskSize, int& gridStart, int& gridStep) const
{
    int miMin = std::max(RedundancyRadius - 1 - i, 0);
    int miMax = std::min(RedundancyRadius - 1 + _gridHeight - 1 - i, MaskSize - 1);
    iMaskSize = miMax - miMin + 1;

    int mjMin = std::max(RedundancyRadius - 1 - j, 0);
    int mjMax = std::min(RedundancyRadius - 1 + _gridWidth - 1 - j, MaskSize - 1);
    jMaskSize = mjMax - mjMin + 1;

    maskStart = miMin * MaskSize + mjMin;
    maskStep = mjMin;

    gridStart = (i - RedundancyRadius + 1 + miMin) * _gridWidth + (j - RedundancyRadius + 1 + mjMin);
    gridStep = _gridWidth - jMaskSize;
}

void MatchSolver::findCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates, const Tiles& tiles) const
{
    int m = 0;
    for (int i = 0; i < _gridHeight; i++)
    {
        for (int j = 0; j < _gridWidth; j++, m++)
        {
            candidates[m].resize(tiles.getNbTiles());

            for (int t = 0; t < tiles.getNbTiles(); t++)
            {
                candidates[m][t]._id = t;
                candidates[m][t]._dist = tiles.computeDistance(i, j, t);
            }

            std::sort(candidates[m].begin(), candidates[m].end());
            candidates[m].resize(_redundancyMaskNbTiles);
        }
    }
}

void MatchSolver::reduceCandidateTiles(std::vector<std::vector<MatchCandidate>>& candidates) const
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
        for (int i = 0; i < _gridHeight; i++)
        {
            for (int j = 0; j < _gridWidth; j++, m++)
            {
                if (lastReduction == m)
                    return;

                if (candidates[m].size() < 2)
                    continue;

                int maskStart, maskStep, iMaskSize, jMaskSize, gridStart, gridStep;
                computeMaskLimits(i, j, maskStart, maskStep, iMaskSize, jMaskSize, gridStart, gridStep);
                for (int t = 0; t < candidates[m].size() - 1; t++)
                {
                    bool redundancy = false;
                    for (int iMask = 0, currMask = maskStart, currId = gridStart; iMask < iMaskSize && !redundancy; iMask++, currMask += maskStep, currId += gridStep)
                    {
                        for (int jMask = 0; jMask < jMaskSize && !redundancy; jMask++, currMask++, currId++)
                        {
                            if (_redundancyMask[currMask] && std::binary_search(sortedId[currId].begin(), sortedId[currId].end(), candidates[m][t]._id))
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

void MatchSolver::findSolution(std::vector<std::vector<MatchCandidate>>& candidates)
{
    std::vector<SortCandidate> sortedCandidates;
    std::set<int> idSet;
    for (int m = 0; m < candidates.size(); m++)
    {
        const int i = m / _gridWidth;
        const int j = m - i * _gridWidth;
        const int start = sortedCandidates.size();
        sortedCandidates.resize(start + candidates[m].size(), SortCandidate(i, j));
        for (int t = 0; t < candidates[m].size(); t++)
        {
            sortedCandidates[start + t]._id = candidates[m][t]._id;
            sortedCandidates[start + t]._dist = candidates[m][t]._dist;
        }
    }
    std::sort(sortedCandidates.begin(), sortedCandidates.end());
   
    _matchingCost = 0;
    cv::Rect box;
    for (int k = 0; k < sortedCandidates.size(); k++)
    {
        int candidateId = sortedCandidates[k]._i * _gridWidth + sortedCandidates[k]._j;
        if (_matchingIds[candidateId] >= 0)
            continue;

        int maskStart, maskStep, iMaskSize, jMaskSize, gridStart, gridStep;
        computeMaskLimits(sortedCandidates[k]._i, sortedCandidates[k]._j, maskStart, maskStep, iMaskSize, jMaskSize, gridStart, gridStep);
        bool redundancy = false;
        for (int iMask = 0, currMask = maskStart, currId = gridStart; iMask < iMaskSize && !redundancy; iMask++, currMask += maskStep, currId += gridStep)
        {
            for (int jMask = 0; jMask < jMaskSize && !redundancy; jMask++, currMask++, currId++)
            {
                if (_redundancyMask[currMask] && _matchingIds[currId] == sortedCandidates[k]._id)
                    redundancy = true;
            }
        }

        if (redundancy)
            continue;

        idSet.insert(sortedCandidates[k]._id).second;
        _matchingIds[candidateId] = sortedCandidates[k]._id;
        _matchingCost += sortedCandidates[k]._dist;
    }
    _uniqueIds.assign(idSet.begin(), idSet.end());

    Log::Logger::get().log(Log::TRACE) << "Matching tiles initial solution found.";
    Log::Logger::get().log(Log::TRACE) << "With mean cost : " << (_matchingCost / (_gridWidth * _gridHeight));
}

