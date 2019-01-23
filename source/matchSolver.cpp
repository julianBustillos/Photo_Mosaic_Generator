#include "matchSolver.h"
#include <opencv2/opencv.hpp>
#include "mathTools.h"
#include "tiles.h"
#include <vector>
#include <iostream>
#include <limits>


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
    cv::Mat imageRes(1960, 4032, CV_8UC3);
    uchar *imageResData = imageRes.data;
    for (int k = 0; k < 1960 * 4032 * 3; k++)
        imageResData[k] = 0;
    //DEBUG

    int k = 0;
    for (int i = 0; i < _subdivisions; i++) {
        for (int j = 0; j < _subdivisions; j++) {
            double features[48];
            cv::Point firstPixel = photo.getFirstPixel(i, j, true);
            MathTools::computeImageFeatures(data, tileSize.width, tileSize.height, firstPixel.y, firstPixel.x, step, 3, features);
            _matchingTiles[k++] = findBestTile(tileData, features);

            //DEBUG
            int blockWidth = (int)ceil(tileSize.width / 4.);
            int blockHeight = (int)ceil(tileSize.height / 4.);
            cv::Point firstPixelNoOffset = photo.getFirstPixel(i, j, false);
            for (int pi = firstPixelNoOffset.y; pi < firstPixelNoOffset.y + tileSize.height; pi++) {
                for (int pj = firstPixelNoOffset.x; pj < firstPixelNoOffset.x + tileSize.width; pj++) {
                    int blockId = 4 * (i / blockHeight) + j / blockWidth;
                    for (int c = 0; c < 3; c++) {
                        imageResData[3 * (pi * 4032 + pj) + c] = (uchar)floor(features[3 * blockId + c]);
                    }
                }
            }
            //DEBUG
        }
    }

    //DEBUG
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\image_debug.jpg", imageRes);
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

int MatchSolver::findBestTile(const std::vector<Tiles::Data>& tileData, const double * features)
{
    double minSquareDistance = std::numeric_limits<double>::max();
    int bestIndex = -1;
    for (unsigned int t = 0; t < tileData.size(); t++) {
        double tempSquareDistance = MathTools::squareDistance(features, tileData[t].features, 48);
        if (tempSquareDistance < minSquareDistance) {
            bestIndex = t;
            minSquareDistance = tempSquareDistance;
        }
    }

    //TODO
    return bestIndex;
}

void MatchSolver::printInfo() const
{
    std::cout << "MATCH SOLVER :" << std::endl;
    std::cout << "Number of tiles to find : " << _subdivisions * _subdivisions << std::endl;
    std::cout << "TODO !!" << std::endl;
    std::cout << std::endl;
}
