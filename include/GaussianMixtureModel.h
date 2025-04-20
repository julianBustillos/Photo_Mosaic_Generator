#pragma once

#include "ProbaUtils.h"
#include <vector>


class GaussianMixtureModel
{
private:
    static const double EpsilonCovariance;

public:
    static bool findOptimalComponents(ProbaUtils::GMMComponents& optimalComponents, const ProbaUtils::CDF& cdf, int color, int nbData, int maxNbComponents, int nbIter, double convergenceTol, bool defaultSeed);

public:
    GaussianMixtureModel(int nbIter, double convergenceTol, bool defaultSeed);
    void setData(const ProbaUtils::CDF& cdf, int color, int nbData);
    bool run(int nbComponents);
    double getBIC();
    ProbaUtils::GMMComponents getComponents();

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
    void runKmeansPlusPlus(int histogramSize, int nbComponents);
    double runExpectationMaximization(int histogramSize, int nbComponents);
    void evalGaussianPDF(std::vector<double>& evals, int histogramSize, int nbComponents) const;
    double logLikelihood(const std::vector<double>& evals, int histogramSize, int nbComponents) const;
    void computeBIC(double logLH, int nbComponents);

private:
    const int _nbIter;
    const double _convergenceTol;
    const bool _defaultSeed;
    std::vector<Bin> _histogram;
    int _nbData;
    ProbaUtils::GMMComponents _components;
    double _BIC;
};

