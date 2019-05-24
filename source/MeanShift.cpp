#include "MeanShift.h"
#include "mathTools.h"
#include <stack>


const int MeanShift::sqSpatialFilter = MEAN_SHIFT_SPATIAL_FILTER * MEAN_SHIFT_SPATIAL_FILTER;
const int MeanShift::sqRangeFilter = MEAN_SHIFT_RANGE_FILTER * MEAN_SHIFT_RANGE_FILTER;
const double MeanShift::sqFirstOptimDist = MEAN_SHIFT_FIRST_OPTIM_DIST * MEAN_SHIFT_FIRST_OPTIM_DIST;
const double MeanShift::sqSecondOptimDist = MEAN_SHIFT_SECOND_OPTIM_DIST * MEAN_SHIFT_SECOND_OPTIM_DIST;
const double MeanShift::sqSpatialMergeRatio = MEAN_SHIFT_SPATIAL_MERGE_RATIO * MEAN_SHIFT_SPATIAL_MERGE_RATIO;
const double MeanShift::sqRangeMergeRatio = MEAN_SHIFT_RANGE_MERGE_RATIO * MEAN_SHIFT_RANGE_MERGE_RATIO;


void MeanShift::compute(const cv::Mat & image, std::vector<int>& clusterMapping, int &nbClusters)
{
    cv::Mat blurredImage(image);
    MathTools::applyGaussianBlur(blurredImage.data, blurredImage.size(), MEAN_SHIFT_BLUR_SIGMA, BLUR_NB_BOXES);

    uchar *blurredImageData = blurredImage.data;
    cv::Size size = image.size();
    std::vector<double[3]> imageLuv(size.width * size.height);
    Data meanShiftData(size.width * size.height);


    int luvId = 0, dataId = 0;
    for (luvId; luvId < imageLuv.size(); luvId++, dataId += 3)
        MathTools::convertBGRtoLUV(imageLuv[luvId][0], imageLuv[luvId][1], imageLuv[luvId][2], blurredImageData[dataId], blurredImageData[dataId + 1], blurredImageData[dataId + 2]);

    std::vector<SpatialOffset> offset;
    offset.push_back(SpatialOffset(0, 0, 0.));
    for (int iWindow = -MEAN_SHIFT_SPATIAL_FILTER; iWindow <= MEAN_SHIFT_SPATIAL_FILTER; iWindow++) {
        for (int jWindow = -MEAN_SHIFT_SPATIAL_FILTER; jWindow <= MEAN_SHIFT_SPATIAL_FILTER; jWindow++) {
            double sqSpatialDist = computeSqSpatialDistance(0, 0, iWindow, jWindow);
            if (sqSpatialDist <= 1. && (iWindow != 0 || jWindow != 0)) {
                offset.push_back(SpatialOffset(iWindow, jWindow, sqSpatialDist));
            }
        }
    }

    std::vector<bool> processed(size.width * size.height, false);
    std::vector<bool> alreadyInConvergencePath(size.width * size.height, false);

    for (int i = 0; i < size.height; i++) {
        for (int j = 0; j < size.width; j++) {
            int currentMeanId = i * size.width + j;
            if (processed[currentMeanId])
                continue;

            WeightedMean weightedMean[2];
            UniqueCoordVector convergencePath(size.width, alreadyInConvergencePath);
            int currentId = 0, previousId = 1;
            getPixelWeightedMean(imageLuv, size, i, j, weightedMean[currentId]);
            int weightedMeanValue = -1;
            double stoppedSqRangeDistance = sqFirstOptimDist + sqSecondOptimDist;
            bool stop = false;

            do {
                previousId = (previousId + 1) % 2;
                currentId = (currentId + 1) % 2;
                int iPrevious = (int)round(weightedMean[previousId]._i);
                int jPrevious = (int)round(weightedMean[previousId]._j);
                weightedMean[currentId].setZero();

                double weightSum = 0.;
                for (int id = 0; id < offset.size(); id++) {
                    SpatialOffset &currentOffset = offset[id];
                    int iWindow = iPrevious + currentOffset._coords._i;
                    int jWindow = jPrevious + currentOffset._coords._j;
                    if (iWindow < 0 || size.height <= iWindow || jWindow < 0 || size.width <= jWindow)
                        continue;

                    WeightedMean windowFeatures;
                    getPixelWeightedMean(imageLuv, size, iWindow, jWindow, windowFeatures);
                    double sqRangeDist = computeSqRangeDistance(windowFeatures, weightedMean[previousId]);
                    double weight = computeKernel(currentOffset._sqSpatialDist + sqRangeDist);

                    weightedMean[currentId] += windowFeatures * weight;
                    weightSum += weight;

                    int meanShiftId = iWindow * size.width + jWindow;
                    if (id == 0) {
                        //First optimisation : Mean shift vector reutilization
                        if (!processed[meanShiftId]) {
                            if (sqRangeDist < sqFirstOptimDist)
                                convergencePath.push_back(Coordinates(iWindow, jWindow));
                        }
                        else {
                            WeightedMean &weightedMeanExisting = meanShiftData.getWeightedMean(meanShiftId);
                            double existingSqRangeDist = computeSqRangeDistance(weightedMeanExisting, weightedMean[previousId]);
                            if (existingSqRangeDist < sqFirstOptimDist) {
                                weightedMeanValue = meanShiftData.getValue(meanShiftId);
                                stoppedSqRangeDistance = existingSqRangeDist;
                                stop = true;
                            }
                        }
                    }
                    else if (weight > 0.) {
                        //Second optimisation : Local neighborhood inclusion
                        if (!processed[meanShiftId]) {
                            if (sqRangeDist < sqSecondOptimDist)
                                convergencePath.push_back(Coordinates(iWindow, jWindow));
                        }
                        else {
                            WeightedMean &weightedMeanExisting = meanShiftData.getWeightedMean(meanShiftId);
                            double existingSqRangeDist = computeSqRangeDistance(weightedMeanExisting, weightedMean[previousId]);
                            if (existingSqRangeDist < sqSecondOptimDist && (existingSqRangeDist < stoppedSqRangeDistance)) {
                                weightedMeanValue = meanShiftData.getValue(meanShiftId);
                                stoppedSqRangeDistance = existingSqRangeDist;
                                stop = true;
                            }
                        }
                    }

                }
                weightedMean[currentId] /= weightSum;

            } while (!stop && !hasConverged(weightedMean[previousId], weightedMean[currentId]));

            if (weightedMeanValue < 0)
                weightedMeanValue = meanShiftData.addWeightedMean(weightedMean[currentId]);

            for (int pathId = 0; pathId < convergencePath.size(); pathId++) {
                int pathPixelId = convergencePath[pathId]._i * size.width + convergencePath[pathId]._j;
                meanShiftData.setValue(pathPixelId, weightedMeanValue);
                processed[pathPixelId] = true;
            }
        }
    }

    meanShiftData.buildClusterMap(clusterMapping, nbClusters);
}

inline void MeanShift::getPixelWeightedMean(const std::vector<double[3]> &imageLuv, cv::Size &size, int i, int j, WeightedMean &weightedMean)
{
    int currentId = i * size.width + j;

    weightedMean._i = (double)i;
    weightedMean._j = (double)j;
    const double *wmLuv = imageLuv[currentId];
    weightedMean._L = wmLuv[0];
    weightedMean._u = wmLuv[1];
    weightedMean._v = wmLuv[2];
}

inline bool MeanShift::hasConverged(WeightedMean & wm1, WeightedMean & wm2)
{
    double iDist = (double)(wm2._i - wm1._i);
    double jDist = (double)(wm2._j - wm1._j);
    double distance = (iDist * iDist + jDist * jDist);

    return distance < 0.25;
}

inline double MeanShift::computeSqSpatialDistance(double i1, double j1, double i2, double j2)
{
    double iDist = (double)(i2 - i1);
    double jDist = (double)(j2 - j1);
    double distance = (iDist * iDist + jDist * jDist) / (double)sqSpatialFilter;

    return distance;
}

inline double MeanShift::computeSqRangeDistance(WeightedMean & wm1, WeightedMean & wm2)
{
    double LDist = wm1._L - wm2._L;
    double uDist = wm1._u - wm2._u;
    double vDist = wm1._v - wm2._v;
    double distance = (LDist * LDist + uDist * uDist + vDist * vDist) / (double)sqRangeFilter;

    return distance;
}

inline double MeanShift::computeKernel(double distance)
{
    return (distance <= 1.) ? 1. - distance * distance : 0.;
}

inline bool MeanShift::canBeMerged(WeightedMean & wm1, WeightedMean & wm2)
{
    if (computeSqSpatialDistance(wm1._i, wm1._j, wm2._i, wm2._j) > sqSpatialMergeRatio)
        return false;

    if (computeSqRangeDistance(wm1, wm2) > sqRangeMergeRatio)
        return false;

    return true;
}

void MeanShift::WeightedMean::setZero()
{
    _i = 0.;
    _j = 0.;
    _L = 0.;
    _u = 0.;
    _v = 0.;
}

MeanShift::WeightedMean & MeanShift::WeightedMean::operator+=(const WeightedMean & rhs)
{
    _i += rhs._i;
    _j += rhs._j;
    _L += rhs._L;
    _u += rhs._u;
    _v += rhs._v;

    return *this;
}

MeanShift::WeightedMean & MeanShift::WeightedMean::operator/=(double rhs)
{
    _i /= rhs;
    _j /= rhs;
    _L /= rhs;
    _u /= rhs;
    _v /= rhs;

    return *this;
}

MeanShift::WeightedMean & MeanShift::WeightedMean::operator*=(double rhs)
{
    _i *= rhs;
    _j *= rhs;
    _L *= rhs;
    _u *= rhs;
    _v *= rhs;

    return *this;
}

MeanShift::WeightedMean & MeanShift::WeightedMean::operator*(double rhs)
{
    _i *= rhs;
    _j *= rhs;
    _L *= rhs;
    _u *= rhs;
    _v *= rhs;

    return *this;
}

void MeanShift::Data::buildClusterMap(std::vector<int>& clusterMapping, int &nbClusters)
{
    std::vector<int> remapping(_weightedMean.size(), -1);
    nbClusters = 0;

    for (int seedId = 0; seedId < remapping.size(); seedId++) {
        if (remapping[seedId] > -1)
            continue;

        std::stack<int> depthFirstSearch;
        depthFirstSearch.push(seedId);

        while (!depthFirstSearch.empty()) {
            int DFSid = depthFirstSearch.top();
            depthFirstSearch.pop();
            remapping[DFSid] = nbClusters;

            for (int compareId = 0; compareId < remapping.size(); compareId++) {
                if (remapping[compareId] > -1)
                    continue;

                if (canBeMerged(_weightedMean[DFSid], _weightedMean[compareId]))
                    depthFirstSearch.push(compareId);
            }
        }

        nbClusters++;
    }

    clusterMapping.resize(_map.size());
    for (int id = 0; id < clusterMapping.size(); id++) {
        clusterMapping[id] = remapping[_map[id]];
    }
}
