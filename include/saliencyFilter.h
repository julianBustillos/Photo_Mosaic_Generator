#pragma once

#include "opencv2/opencv.hpp"
#include <vector>


class SaliencyFilter
{
private:
    static constexpr double UniquenessSigma = 300.;
    static constexpr double DistributionSigma = 2.;
    static constexpr double DistributionInfluence = 6.;
    static constexpr double RegionMaxSize = 0.3;
    static constexpr double MinThreshold = 0.05;

    static constexpr double uniquenessCoefficient = 1. / (2. * UniquenessSigma * UniquenessSigma);
    static constexpr double distributionCoefficient = 1. / (2. * DistributionSigma * DistributionSigma);

public:
    static void compute(const cv::Mat& image, const std::vector<int>& clusterMapping, int nbClusters, std::vector<double>& saliency, int& iMean, int& jMean, bool& saliencyFound);

private:
    struct ClusterPoint
    {
        double _i = 0.;
        double _j = 0.;
        double _L = 0.;
        double _a = 0.;
        double _b = 0.;
        int _B = 0;
        int _G = 0;
        int _R = 0;
        int _size = 0;
        double _uniqueness = 0.;
        double _distribution = 0.;
        double _saliency = 0.;
    };

private:
    static void buildClusters(std::vector<ClusterPoint>& cluster, const cv::Mat& image, const std::vector<int>& clusterMapping);
    static void computeColorSqDistance(const std::vector<ClusterPoint>& cluster, std::vector<double>& colorSqDistance);
    static void computeUniqueness(const std::vector<double>& colorSqDistance, std::vector<ClusterPoint>& cluster);
    static void computeDistribution(const std::vector<double>& colorSqDistance, std::vector<ClusterPoint>& cluster);
    static void computeSaliency(std::vector<ClusterPoint>& cluster);
    static void findSaliencyThreshold(const std::vector<ClusterPoint>& cluster, double& threshold);
    static void findMeanClusterPoint(const std::vector<ClusterPoint>& cluster, double threshold, int& iMean, int& jMean);
    static void buildThresholdedSaliency(const std::vector<ClusterPoint>& cluster, std::vector<double>& saliency, double threshold);

private:
    SaliencyFilter() {};
    ~SaliencyFilter() {};
};