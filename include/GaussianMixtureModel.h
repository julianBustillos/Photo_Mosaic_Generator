#pragma once

#include "ProbaUtils.h"
#include "MathUtils.h"
#include <vector>
#include <random>
#include <map>
#include <set>
#include <numbers>


template<unsigned int N>
class GaussianMixtureModel
{
private:
    static const double EpsilonCovariance;
    static const double EpsilonDetCovariance;

public:
    static bool findOptimalComponents(ProbaUtils::GMMNDComponents<N>& optimalComponents, const ProbaUtils::SampleData<N>& sampleData, int minNbComponents, int maxNbComponents, int nbIter, double convergenceTol, bool defaultSeed);

public:
    GaussianMixtureModel(const ProbaUtils::SampleData<N>& sampleData, int nbIter, double convergenceTol, bool defaultSeed);
    bool run(int nbComponents);
    double getBIC();
    ProbaUtils::GMMNDComponents<N> getComponents();

private:
    struct ClusterData
    {
        MathUtils::VectorNd<N> _sum;
        int _count;
    };

private:
    bool checkValidity(int nbComponents);
    void runKmeansPlusPlus(int histogramSize, int nbComponents);
    double runExpectationMaximization(int histogramSize, int nbComponents);
    double logLikelihood(const std::vector<double>& evals, int histogramSize, int nbComponents) const;
    void computeBIC(double logLH, int nbComponents);

private:
    const int _nbIter;
    const double _convergenceTol;
    const bool _defaultSeed;
    const ProbaUtils::SampleData<N>& _sampleData;
    ProbaUtils::GMMNDComponents<N> _components;
    double _BIC;
};


template<unsigned int N>
const double GaussianMixtureModel<N>::EpsilonCovariance = 1. / 16.;

template<unsigned int N>
const double GaussianMixtureModel<N>::EpsilonDetCovariance = MathUtils::pow(GaussianMixtureModel<N>::EpsilonCovariance, N);

template<unsigned int N>
bool GaussianMixtureModel<N>::findOptimalComponents(ProbaUtils::GMMNDComponents<N>& optimalComponents, const ProbaUtils::SampleData<N>& sampleData, int minNbComponents, int maxNbComponents, int nbIter, double convergenceTol, bool defaultSeed)
{
    GaussianMixtureModel<N> gmm(sampleData, nbIter, convergenceTol, defaultSeed);
    double optimalBIC = MathUtils::DoubleMax;
    bool found = false;

    for (int nbComponents = minNbComponents; nbComponents <= maxNbComponents; nbComponents++)
    {
        found = gmm.run(nbComponents);
        if (found)
        {
            double BIC = gmm.getBIC();
            if (BIC < optimalBIC)
            {
                optimalComponents = gmm.getComponents();
                optimalBIC = BIC;
            }
        }
    }

    return found;
}

template<unsigned int N>
GaussianMixtureModel<N>::GaussianMixtureModel(const ProbaUtils::SampleData<N>& sampleData, int nbIter, double convergenceTol, bool defaultSeed) :
    _sampleData(sampleData), _nbIter(nbIter), _convergenceTol(convergenceTol), _defaultSeed(defaultSeed), _BIC(MathUtils::DoubleMax)
{
}

template<unsigned int N>
bool GaussianMixtureModel<N>::run(int nbComponents)
{
    if (!checkValidity(nbComponents))
        return false;

    _components.resize(nbComponents);

    runKmeansPlusPlus(_sampleData._histogram.size(), nbComponents);
    double logLH = runExpectationMaximization(_sampleData._histogram.size(), nbComponents);
    computeBIC(logLH, nbComponents);

    return true;
}

template<unsigned int N>
double GaussianMixtureModel<N>::getBIC()
{
    return _BIC;
}

template<unsigned int N>
ProbaUtils::GMMNDComponents<N> GaussianMixtureModel<N>::getComponents()
{
    return _components;
}

template<unsigned int N>
bool GaussianMixtureModel<N>::checkValidity(int nbComponents)
{
    return nbComponents > 0 && _sampleData._histogram.size() >= nbComponents;
}

template<unsigned int N>
void GaussianMixtureModel<N>::runKmeansPlusPlus(int histogramSize, int nbComponents)
{
    //Use kmeans++ as initializer for GMM
    std::random_device rd;
    std::unique_ptr<std::mt19937> gen = _defaultSeed ? std::make_unique<std::mt19937>() : std::make_unique<std::mt19937>(rd());

    //Choose initial clusters (means)
    std::uniform_int_distribution<> uniformDistrib(0, histogramSize - 1);
    _components[0]._mean = _sampleData._histogram[uniformDistrib(*gen.get())]._value;

    std::vector<int> intervals(2 * histogramSize);
    std::vector<double> weights(intervals.size() - 1, 0);
    for (int b = 0; b < histogramSize; b++)
    {
        intervals[2 * b] = b;
        intervals[2 * b + 1] = b + 1;
        weights[2 * b] = MathUtils::IntMax;
    }

    for (int c = 1; c < nbComponents; c++)
    {
        for (int b = 0; b < histogramSize; b++)
        {
            int sqDistance = MathUtils::sqDistance(_components[c - 1]._mean, _sampleData._histogram[b]._value);
            if (sqDistance < weights[2 * b])
                weights[2 * b] = sqDistance;
        }

        std::piecewise_constant_distribution<> pieceConstDistrib(intervals.begin(), intervals.end(), weights.begin());
        _components[c]._mean = _sampleData._histogram[pieceConstDistrib(*gen.get())]._value;
    }

    //Iteration step
    std::vector<ClusterData> clusters(nbComponents);
    std::vector<int> assignedCluster(histogramSize, -1);
    double sqMeanMaxDiff = MathUtils::DoubleMax;
    int iteration = 0;

    while (sqMeanMaxDiff > _convergenceTol && iteration < _nbIter)
    {
        for (ClusterData& cluster : clusters)
        {
            cluster._sum.setZero();
            cluster._count = 0;
        }

        //Assign each point to closest cluster centroid
        for (int b = 0; b < histogramSize; b++)
        {
            double sqDistanceMin = MathUtils::DoubleMax;
            for (int c = 0; c < nbComponents; c++)
            {
                double sqDistance = MathUtils::sqDistance(_components[c]._mean, _sampleData._histogram[b]._value);
                if (sqDistance < sqDistanceMin)
                {
                    assignedCluster[b] = c;
                    sqDistanceMin = sqDistance;
                }
            }
            clusters[assignedCluster[b]]._sum += _sampleData._histogram[b]._value * (double)_sampleData._histogram[b]._count;
            clusters[assignedCluster[b]]._count += _sampleData._histogram[b]._count;
        }

        //Compute new centroids
        sqMeanMaxDiff = 0;
        for (int c = 0; c < nbComponents; c++)
        {
            MathUtils::VectorNd<N> newCentroid = clusters[c]._sum / (double)clusters[c]._count;
            double sqMeanDiff = MathUtils::sqDistance(_components[c]._mean, newCentroid);
            if (sqMeanDiff > sqMeanMaxDiff)
                sqMeanMaxDiff = sqMeanDiff;

            _components[c]._mean = newCentroid;
        }

        iteration++;
    }

    //Initialize variances and weights
    for (int b = 0; b < histogramSize; b++)
    {
        const int cluster = assignedCluster[b];
        _components[cluster]._covariance += (_sampleData._histogram[b]._value - _components[cluster]._mean) * (_sampleData._histogram[b]._value - _components[cluster]._mean).transpose() * (double)_sampleData._histogram[b]._count;
    }
    for (int c = 0; c < nbComponents; c++)
    {
        _components[c]._covariance = _components[c]._covariance / (double)clusters[c]._count;
        if (_components[c]._covariance.determinant() < EpsilonDetCovariance)
        {
            _components[c]._covariance.setIdentity();
            _components[c]._covariance *= EpsilonCovariance;
        }
        _components[c]._weight = 1. / (double)nbComponents;
    }
}

template<unsigned int N>
double GaussianMixtureModel<N>::runExpectationMaximization(int histogramSize, int nbComponents)
{
    std::vector<double> evals(nbComponents * histogramSize); //Gaussian PDF evaluations
    std::vector<double> resps(nbComponents * histogramSize); //Responsabilities
    std::vector<double> composResp(nbComponents);
    int iteration = 0;
    ProbaUtils::evalGaussianPDF(evals, _sampleData, _components);
    double logLH = logLikelihood(evals, histogramSize, nbComponents);
    double logLHDiff = MathUtils::DoubleMax; //Log-likelihood iteration difference

    while (logLHDiff > _convergenceTol && iteration < _nbIter)
    {
        //Expectation step
        for (int b = 0, r = 0; b < histogramSize; b++)
        {
            double respAcc = 0;
            for (int c = 0; c < nbComponents; c++, r++)
            {
                respAcc += evals[r];
            }
            r -= nbComponents;
            for (int c = 0; c < nbComponents; c++, r++)
            {
                resps[r] = (evals[r] * (double)_sampleData._histogram[b]._count) / respAcc;
            }
        }

        //Maximization step
        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._mean.setZero();
            _components[c]._covariance.setZero();
            composResp[c] = 0;
        }

        for (int b = 0, r = 0; b < histogramSize; b++)
        {
            for (int c = 0; c < nbComponents; c++, r++)
            {
                composResp[c] += resps[r];
                _components[c]._mean += resps[r] * _sampleData._histogram[b]._value;
            }
        }

        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._mean /= composResp[c];
        }

        for (int b = 0, r = 0; b < histogramSize; b++)
        {
            for (int c = 0; c < nbComponents; c++, r++)
            {
                _components[c]._covariance += resps[r] * (_sampleData._histogram[b]._value - _components[c]._mean) * (_sampleData._histogram[b]._value - _components[c]._mean).transpose();
            }

        }

        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._covariance /= composResp[c];
            if (_components[c]._covariance.determinant() < EpsilonDetCovariance)
            {
                _components[c]._covariance.setIdentity();
                _components[c]._covariance *= EpsilonCovariance;
            }

            _components[c]._weight = composResp[c] / _sampleData._nbData;
        }

        //Compute log-likelihood
        ProbaUtils::evalGaussianPDF(evals, _sampleData, _components);
        double newLogLH = logLikelihood(evals, histogramSize, nbComponents);
        logLHDiff = newLogLH - logLH;
        logLH = newLogLH;

        iteration++;
    }

    return logLH;
}

template<unsigned int N>
double GaussianMixtureModel<N>::logLikelihood(const std::vector<double>& evals, int histogramSize, int nbComponents) const
{
    double logLH = 0;
    for (int b = 0, e = 0; b < histogramSize; b++)
    {
        double binEval = 0;
        for (int c = 0; c < nbComponents; c++, e++)
        {
            binEval += evals[e];
        }
        logLH += log(binEval) * (double)_sampleData._histogram[b]._count;
    }

    return logLH;
}

template<unsigned int N>
void GaussianMixtureModel<N>::computeBIC(double logLH, int nbComponents)
{
    _BIC = -2. * logLH + (double)(3 * nbComponents - 1) * log(_sampleData._nbData);
}