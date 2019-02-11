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
    _matchingTiles = new int[_subdivisions * _subdivisions];
    if (!_matchingTiles)
        return;

    memset(_matchingTiles, -1, _subdivisions * _subdivisions * sizeof(int));

    const uchar * data = photo.getData();
    const cv::Size tileSize = photo.getTileSize();
    const int step = photo.getStep();
    const std::vector<Tiles::Data> &tileData = tiles.getTileDataVector();

    //DEBUG
   /* cv::Mat imageRes(1960, 4032, CV_8UC3);
    uchar *imageResData = imageRes.data;
    for (int k = 0; k < 1960 * 4032 * 3; k++)
        imageResData[k] = 0;*/
    //DEBUG

    std::vector<matchCandidate> candidates;

    //int k = 0;
    for (int i = 0; i < _subdivisions; i++) {
        for (int j = 0; j < _subdivisions; j++) {
            double features[3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION];
            cv::Point firstPixel = photo.getFirstPixel(i, j, true);
            MathTools::computeImageFeatures(data, tileSize.width, tileSize.height, firstPixel.y, firstPixel.x, step, 3, features, FEATURE_ROOT_SUBDIVISION);
            findCandidateTiles(candidates, i, j, tileData, features);

            //DEBUG
            /*int blockWidth = (int)ceil(tileSize.width / 4.);
            int blockHeight = (int)ceil(tileSize.height / 4.);
            cv::Point firstPixelNoOffset = photo.getFirstPixel(i, j, false);
            for (int pi = firstPixelNoOffset.y; pi < firstPixelNoOffset.y + tileSize.height; pi++) {
                for (int pj = firstPixelNoOffset.x; pj < firstPixelNoOffset.x + tileSize.width; pj++) {
                    int blockId = 4 * (i / blockHeight) + j / blockWidth;
                    for (int c = 0; c < 3; c++) {
                        imageResData[3 * (pi * 4032 + pj) + c] = (uchar)floor(features[3 * blockId + c]);
                    }
                }
            }*/
            //DEBUG
        }
    }

    findBestTiles(candidates);

    //DEBUG
    //cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\image_debug.jpg", imageRes);
    //DEBUG

    printInfo();
}

MatchSolver::~MatchSolver()
{
    if (_matchingTiles)
        delete _matchingTiles;
    _matchingTiles = 0;
}

int * MatchSolver::getMatchingTiles()
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
        temp._squareDistance = MathTools::squareDistance(features, tileData[t].features, 3 * FEATURE_ROOT_SUBDIVISION * FEATURE_ROOT_SUBDIVISION);
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
