#pragma once

#include <vector>


class GaussianMixtureModel
{
private:
    static double square(double value);
    static const double EpsilonCovariance;

public:
    struct Component
    {
        double _mean;
        double _covariance;
        double _weight;
    };

public:
    static std::vector<Component> findOptimalComponents(const std::vector<int>& data, int maxNbComponents, double kmeansTol, int kmeansIter, double emTol, int emIter, bool defaultSeed);

public:
    GaussianMixtureModel(double kmeansTol, int kmeansIter, double emTol, int emIter, bool defaultSeed);
    void setData(const std::vector<int>& data);
    bool run(int nbComponents);
    double getBIC();
    std::vector<Component> getComponents();

private:
    struct Bin
    {
        int _value;
        int _count;
    };

    struct ClusterData
    {
        int _sum;
        int _count;
    };

private:
    bool checkValidity(int nbComponents);
    void runKmeansPlusPlus();
    void runExpectationMaximization();
    double normalPDF(double value, const Component& component);
    double logLikelihood();
    void computeBIC();

private:
    const double _kmeansTol;
    const int _kmeansIter;
    const double _emTol;
    const int _emIter;
    const bool _defaultSeed;
    std::vector<Bin> _histogram;
    int _nbData;
    std::vector<Component> _components;
    double _BIC;
};

