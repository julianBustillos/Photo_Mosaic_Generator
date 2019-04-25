#include "matchSolver.h"
#include "variables.h"
#include <opencv2/opencv.hpp>
#include "mathTools.h"
#include "tiles.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include "sortedVector.h"


MatchSolver::MatchSolver(const Photo &photo, const Tiles &tiles, int subdivisions) :
    _matchingTiles(0), _subdivisions(subdivisions)
{
    _matchingTiles.resize(_subdivisions * _subdivisions, -1);

    const uchar * data = photo.getData();
    const cv::Size tileSize = photo.getTileSize();
    const int step = photo.getStep();
    const std::vector<Tiles::Data> &tileData = tiles.getTileDataVector();

    std::vector<matchCandidate> candidates;

    for (int i = 0; i < _subdivisions; i++) {
        for (int j = 0; j < _subdivisions; j++) {
            double features[3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION];
            cv::Point firstPixel = photo.getFirstPixel(i, j, true);
            MathTools::computeImageBGRFeatures(data, tileSize, firstPixel, step, features, FEATURE_ROOT_SUBDIVISION);
            findCandidateTiles(candidates, i, j, tileData, features);
        }
    }

    findBestTiles(candidates);

    printInfo();
}

const std::vector<int> &MatchSolver::getMatchingTiles()
{
    return _matchingTiles;
}

void MatchSolver::findCandidateTiles(std::vector<matchCandidate>& candidates, int i, int j, const std::vector<Tiles::Data>& tileData, const double * features)
{
    SortedVector<matchCandidate> tileCandidates(_redundancyTilesNumber);
    matchCandidate temp;
    temp._i = i;
    temp._j = j;

    for (unsigned int t = 0; t < tileData.size(); t++) {
        temp._id = t;
        temp._squareDistance = MathTools::BGRFeatureDistance(features, tileData[t].features, FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION);
        tileCandidates.push_sorted(temp);
    }

    for (int k = 0; k < tileCandidates.size(); k++)
        candidates.push_back(tileCandidates[k]);
}

void MatchSolver::findBestTiles(std::vector<matchCandidate>& candidates)
{
    std::sort(candidates.begin(), candidates.end());
    for (int k = 0; k < candidates.size(); k++) {
        int candidateId = candidates[k]._i * _subdivisions + candidates[k]._j;
        if (_matchingTiles[candidateId] >= 0)
            continue;

        int iMin = std::max(candidates[k]._i - REDUNDANCY_DISTANCE, 0);
        int iMax = std::min(candidates[k]._i + REDUNDANCY_DISTANCE, _subdivisions - 1);
        int jMin = std::max(candidates[k]._j - REDUNDANCY_DISTANCE, 0);
        int jMax = std::min(candidates[k]._j + REDUNDANCY_DISTANCE, _subdivisions - 1);

        bool redundancy = false;
        for (int i = iMin; i <= iMax && !redundancy; i++) {
            for (int j = jMin; j <= jMax && !redundancy; j++) {
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

void MatchSolver::printInfo() const
{
    std::cout << "MATCH SOLVER :" << std::endl;
    std::cout << "Number of tiles to find : " << _subdivisions * _subdivisions << std::endl;
    std::cout << "TODO (distance ?) !!" << std::endl;
    std::cout << std::endl;
}
