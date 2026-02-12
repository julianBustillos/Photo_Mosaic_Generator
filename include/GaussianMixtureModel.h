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
public:
    static bool findOptimalComponents(ProbaUtils::GMMNDComponents<N>& optimalComponents, const ProbaUtils::Histogram<N>& histogram, int minNbComponents, int maxNbComponents, int nbInit, int nbIter, double convergenceTol, double covarianceReg, bool defaultSeed);

public:
    GaussianMixtureModel(const ProbaUtils::Histogram<N>& histogram, int nbInit, int nbIter, double convergenceTol, double covarianceReg, bool defaultSeed);
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
    void regularizeCovariance(double* covarianceData);
    void runKmeansPlusPlus(int nbComponents);
    double runExpectationMaximization(int nbComponents);
    double logLikelihood(const std::vector<double>& norms, int nbComponents) const;
    void computeBIC(double logLH, int nbComponents);

private:
    const int _nbInit;
    const int _nbIter;
    const double _convergenceTol;
    const double _covarianceReg;
    const bool _defaultSeed;
    const ProbaUtils::Histogram<N>& _histogram;
    ProbaUtils::GMMNDComponents<N> _components;
    double _BIC;
};


template<unsigned int N>
bool GaussianMixtureModel<N>::findOptimalComponents(ProbaUtils::GMMNDComponents<N>& optimalComponents, const ProbaUtils::Histogram<N>& histogram, int minNbComponents, int maxNbComponents, int nbInit, int nbIter, double convergenceTol, double covarianceReg, bool defaultSeed)
{
    GaussianMixtureModel<N> gmm(histogram, nbInit, nbIter, convergenceTol, covarianceReg, defaultSeed);
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
GaussianMixtureModel<N>::GaussianMixtureModel(const ProbaUtils::Histogram<N>& histogram, int nbInit, int nbIter, double convergenceTol, double covarianceReg, bool defaultSeed) :
    _histogram(histogram), _nbInit(nbInit), _nbIter(nbIter), _convergenceTol(convergenceTol), _defaultSeed(defaultSeed), _covarianceReg(covarianceReg), _BIC(MathUtils::DoubleMax)
{
}

template<unsigned int N>
bool GaussianMixtureModel<N>::run(int nbComponents)
{
    if (!checkValidity(nbComponents))
        return false;

    _components.resize(nbComponents);

    runKmeansPlusPlus(nbComponents);
    double logLH = runExpectationMaximization(nbComponents);
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
    return nbComponents > 0 && _histogram._values.size() == _histogram._counts.size() && _histogram._values.size() >= nbComponents && _nbInit > 0;
}

template<unsigned int N>
inline void GaussianMixtureModel<N>::regularizeCovariance(double* covarianceData)
{
    for (int d = 0, i = 0; d < N; d++, i += N + 1)
        covarianceData[i] += _covarianceReg;
}

template<unsigned int N>
void GaussianMixtureModel<N>::runKmeansPlusPlus(int nbComponents)
{
    const int nbValues = _histogram._values.size();

    //Use kmeans++ as initializer for GMM
    std::unique_ptr<std::mt19937> gen = _defaultSeed ? std::make_unique<std::mt19937>() : std::make_unique<std::mt19937>(std::random_device{}());
    std::uniform_int_distribution<int> uniform(0, nbValues - 1);

    double bestInertia = MathUtils::DoubleMax;
    std::vector<ClusterData> bestClusters(nbComponents);
    std::vector<int> bestAssignedCluster(nbValues, -1);
    std::vector<int> intervals(2 * nbValues);
    std::vector<double> weights(intervals.size() - 1, 0);
    MathUtils::VectorNd<N> meanValDiff;

    for (int i = 0; i < _nbInit; i++)
    {
        ProbaUtils::GMMNDComponents<N> components(nbComponents);

        //Choose initial clusters (means)
        components[0]._mean = _histogram._values[uniform(*gen.get())];

        std::fill(weights.begin(), weights.end(), 0);
        for (int b = 0; b < nbValues; b++)
        {
            intervals[2 * b] = b;
            intervals[2 * b + 1] = b + 1;
            weights[2 * b] = MathUtils::IntMax;
        }

        for (int c = 1; c < nbComponents; c++)
        {
            for (int b = 0; b < nbValues; b++)
            {
                MathUtils::VectorNd<N> meanValDiff = components[c - 1]._mean - _histogram._values[b];
                int sqDistance = 0;
                for (int i = 0; i < N; i++)
                    sqDistance += meanValDiff.data()[i] * meanValDiff.data()[i];
                if (sqDistance < weights[2 * b])
                    weights[2 * b] = sqDistance;
            }

            std::piecewise_constant_distribution<> pieceConstDistr(intervals.begin(), intervals.end(), weights.begin());
            components[c]._mean = _histogram._values[pieceConstDistr(*gen.get())];
        }

        //Iteration step
        std::vector<ClusterData> clusters(nbComponents);
        std::vector<int> assignedCluster(nbValues, -1);
        double inertia = MathUtils::DoubleMax;
        double sqDistanceMax = MathUtils::DoubleMax;
        int iteration = 0;

        while (sqDistanceMax > _convergenceTol && iteration < _nbIter)
        {
            for (ClusterData& cluster : clusters)
            {
                cluster._sum.setZero();
                cluster._count = 0;
            }

            //Assign each point to closest cluster centroid
            inertia = 0;
            for (int b = 0; b < nbValues; b++)
            {
                double sqDistanceMin = MathUtils::DoubleMax;
                for (int c = 0; c < nbComponents; c++)
                {
                    meanValDiff = components[c]._mean - _histogram._values[b];
                    int sqDistance = 0;
                    for (int i = 0; i < N; i++)
                        sqDistance += meanValDiff.data()[i] * meanValDiff.data()[i];
                    if (sqDistance < sqDistanceMin)
                    {
                        assignedCluster[b] = c;
                        sqDistanceMin = sqDistance;
                    }
                }
                clusters[assignedCluster[b]]._sum += _histogram._values[b] * (double)_histogram._counts[b];
                clusters[assignedCluster[b]]._count += _histogram._counts[b];
                inertia += sqDistanceMin;
            }

            //Compute new centroids
            sqDistanceMax = 0;
            for (int c = 0; c < nbComponents; c++)
            {
                MathUtils::VectorNd<N> newCentroid = clusters[c]._sum / (double)clusters[c]._count;
                meanValDiff = components[c]._mean - newCentroid;
                int sdDistance = 0;
                for (int i = 0; i < N; i++)
                    sdDistance += meanValDiff.data()[i] * meanValDiff.data()[i];

                if (sdDistance > sqDistanceMax)
                    sqDistanceMax = sdDistance;

                components[c]._mean = newCentroid;
            }

            iteration++;
        }

        if (inertia < bestInertia)
        {
            bestInertia = inertia;
            bestClusters = clusters;
            bestAssignedCluster = assignedCluster;
            _components = components;
        }
    }

    //Initialize variances and weights
    for (int b = 0; b < nbValues; b++)
    {
        const int cluster = bestAssignedCluster[b];
        meanValDiff = _histogram._values[b] - _components[cluster]._mean;
        _components[cluster]._covariance += meanValDiff * meanValDiff.transpose() * (double)_histogram._counts[b];
    }
    for (int c = 0; c < nbComponents; c++)
    {
        _components[c]._covariance = _components[c]._covariance / (double)bestClusters[c]._count;
        regularizeCovariance(_components[c]._covariance.data());

        _components[c]._weight = 1. / (double)nbComponents;
    }
}

template<unsigned int N>
double GaussianMixtureModel<N>::runExpectationMaximization(int nbComponents)
{
    const int nbValues = _histogram._values.size();

    std::vector<double> probas(nbComponents * nbValues); //Gaussian PDF probabilities
    std::vector<double> norms(nbValues); //Gaussian PDF density norms
    std::vector<double> resps(nbComponents * nbValues); //Responsabilities
    std::vector<double> composResp(nbComponents);
    int iteration = 0;
    ProbaUtils::evalGaussianPDF(probas, norms, _histogram._values, _components, true);
    double logLH = logLikelihood(norms, nbComponents);
    double logLHDiff = MathUtils::DoubleMax; //Log-likelihood iteration difference
    MathUtils::VectorNd<N> meanValDiff;

    while (logLHDiff > _convergenceTol && iteration < _nbIter)
    {
        //Expectation step
        for (int b = 0, r = 0; b < nbValues; b++)
            for (int c = 0; c < nbComponents; c++, r++)
                resps[r] = probas[r] * (double)_histogram._counts[b];

        //Maximization step
        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._mean.setZero();
            _components[c]._covariance.setZero();
            composResp[c] = 0;
        }

        for (int b = 0, r = 0; b < nbValues; b++)
        {
            for (int c = 0; c < nbComponents; c++, r++)
            {
                composResp[c] += resps[r];
                _components[c]._mean += resps[r] * _histogram._values[b];
            }
        }

        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._mean /= composResp[c];
        }

        for (int b = 0, r = 0; b < nbValues; b++)
        {
            for (int c = 0; c < nbComponents; c++, r++)
            {
                meanValDiff = _histogram._values[b] - _components[c]._mean;
                for (int i = 0; i < N; i++)
                    for (int j = 0; j < N; j++)
                        _components[c]._covariance(i, j) += resps[r] * meanValDiff.data()[i] * meanValDiff.data()[j];
            }

        }

        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._covariance /= composResp[c];
            regularizeCovariance(_components[c]._covariance.data());

            _components[c]._weight = composResp[c] / _histogram._nbData;
        }

        //Compute log-likelihood
        ProbaUtils::evalGaussianPDF(probas, norms, _histogram._values, _components, true);
        double newLogLH = logLikelihood(norms, nbComponents);
        logLHDiff = newLogLH - logLH;
        logLH = newLogLH;

        iteration++;
    }

    return logLH;
}

template<unsigned int N>
double GaussianMixtureModel<N>::logLikelihood(const std::vector<double>& norms, int nbComponents) const
{
    const int nbValues = _histogram._counts.size();

    double logLH = 0;
    for (int b = 0, e = 0; b < nbValues; b++)
        logLH += log(norms[b]) * (double)_histogram._counts[b];

    return logLH;
}

template<unsigned int N>
void GaussianMixtureModel<N>::computeBIC(double logLH, int nbComponents)
{
    _BIC = -2. * logLH + (double)(3 * nbComponents - 1) * log(_histogram._nbData);
}