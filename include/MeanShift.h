#pragma once

#include "opencv2/opencv.hpp"
#include <vector>


class MeanShift
{
private:
    static constexpr int BlurSigma = 8;
    static constexpr int SpatialFilter = 64;
    static constexpr int RangeFilter = 20;
    static constexpr double FirstOptimDist = 0.5;
    static constexpr double SecondOptimDist = 0.1;
    static constexpr double SpatialMergeRatio = 0.5;
    static constexpr double RangeMergeRatio = 0.5;

    static constexpr int sqSpatialFilter = SpatialFilter * SpatialFilter;
    static constexpr int sqRangeFilter = RangeFilter * RangeFilter;
    static constexpr double sqFirstOptimDist = FirstOptimDist * FirstOptimDist;
    static constexpr double sqSecondOptimDist = SecondOptimDist * SecondOptimDist;
    static constexpr double sqSpatialMergeRatio = SpatialMergeRatio * SpatialMergeRatio;
    static constexpr double sqRangeMergeRatio = RangeMergeRatio * RangeMergeRatio;

public:
    static void compute(const cv::Mat& image, std::vector<int>& clusterMapping, int& nbClusters);

private:
    struct Coordinates
    {
        Coordinates(int i, int j) : _i(i), _j(j) {};

        int _i;
        int _j;
    };

    struct LUV
    {
        double _L;
        double _u;
        double _v;
    };

    struct SpatialOffset
    {
        SpatialOffset(int i, int j, double sqSpatialDist) : _coords(i, j), _sqSpatialDist(sqSpatialDist) {};

        Coordinates _coords;
        double _sqSpatialDist;
    };

    class UniqueCoordVector
    {
    public:
        UniqueCoordVector(int width, std::vector<bool>& alreadyExist) : _width(width), _alreadyExist(alreadyExist) { _vector.reserve(width); };
        void emplace_back(const Coordinates& coord)
        {
            int index = coord._i * _width + coord._j;
            if (!_alreadyExist[index])
            {
                _vector.emplace_back(coord);
                _alreadyExist[index] = true;
            }
        };
        size_t size() const { return _vector.size(); };
        Coordinates& operator[](int pos)
        {
            return _vector[pos];
        };

    private:
        const int _width;
        std::vector<bool>& _alreadyExist;
        std::vector<Coordinates> _vector;
    };

    struct WeightedMean
    {
        double _i;
        double _j;
        double _L;
        double _u;
        double _v;

        void setZero();

        WeightedMean& operator+=(const WeightedMean& rhs);
        WeightedMean& operator/=(double rhs);
        WeightedMean& operator*=(double rhs);
        WeightedMean& operator*(double rhs);
    };

    class Data
    {
    public:
        Data(int size) : _map(size, -1) { _weightedMean.reserve(size / 4); };
        int getValue(int key) { return _map[key]; };
        void setValue(int key, int value) { _map[key] = value; };
        WeightedMean& getWeightedMean(int key) { return _weightedMean[_map[key]]; };
        int addWeightedMean(WeightedMean& weightedMean) { _weightedMean.emplace_back(weightedMean); return (int)_weightedMean.size() - 1; };
        void buildClusterMap(std::vector<int>& clusterMapping, int& nbClusters);

    private:
        std::vector<WeightedMean> _weightedMean;
        std::vector<int> _map;
    };

private:
    static inline void getPixelWeightedMean(const std::vector<LUV>& imageLuv, cv::Size& size, int i, int j, WeightedMean& weightedMean);
    static inline bool hasConverged(WeightedMean& wm1, WeightedMean& wm2);
    static inline double computeSqSpatialDistance(double i1, double j1, double i2, double j2);
    static inline double computeSqRangeDistance(WeightedMean& wm1, WeightedMean& wm2);
    static inline double computeKernel(double distance);
    static inline bool canBeMerged(WeightedMean& wm1, WeightedMean& wm2);
};