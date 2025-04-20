#include "GaussianMixtureModel.h"
#include "MathUtils.h"
#include <random>
#include <set>
#include <numbers>


const double GaussianMixtureModel::EpsilonCovariance = 1. / 16.;

bool GaussianMixtureModel::findOptimalComponents(ProbaUtils::GMMComponents& optimalComponents, const ProbaUtils::CDF& cdf, int color, int nbData, int maxNbComponents, int nbIter, double convergenceTol, bool defaultSeed)
{
    GaussianMixtureModel gmm(nbIter, convergenceTol, defaultSeed);
    gmm.setData(cdf, color, nbData);
    double optimalBIC = MathUtils::DoubleMax;
    bool found = false;

    for (int nbComponents = 1; nbComponents <= maxNbComponents; nbComponents++)
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

GaussianMixtureModel::GaussianMixtureModel(int nbIter, double convergenceTol,  bool defaultSeed) :
    _nbIter(nbIter), _convergenceTol(convergenceTol), _defaultSeed(defaultSeed), _nbData(0), _BIC(0)
{
}

void GaussianMixtureModel::setData(const ProbaUtils::CDF& cdf, int color, int nbData)
{
    _histogram.clear();
    _components.clear();
    _BIC = MathUtils::DoubleMax;

    for (int value = 0; value < 256; value++)
    {
        double density = value ? cdf[color][value] - cdf[color][value - 1] : cdf[color][value];
        if (density > 0)
        {
            _histogram.emplace_back(value, (int)std::round(density * (double)nbData));
        }
    }
    _nbData = nbData;
}

bool GaussianMixtureModel::run(int nbComponents)
{
    if (!checkValidity(nbComponents))
        return false;

    _components.resize(nbComponents);

    runKmeansPlusPlus(_histogram.size(), nbComponents);
    double logLH = runExpectationMaximization(_histogram.size(), nbComponents);
    computeBIC(logLH, nbComponents);
  
    return true;
}

double GaussianMixtureModel::getBIC()
{
    return _BIC;
}

ProbaUtils::GMMComponents GaussianMixtureModel::getComponents()
{
    return _components;
}

bool GaussianMixtureModel::checkValidity(int nbComponents)
{
    return nbComponents > 0 && _histogram.size() >= nbComponents;
}

void GaussianMixtureModel::runKmeansPlusPlus(int histogramSize, int nbComponents)
{
    //Use kmeans++ as initializer for GMM
    std::random_device rd;
    std::unique_ptr<std::mt19937> gen = _defaultSeed ? std::make_unique<std::mt19937>() : std::make_unique<std::mt19937>(rd());

    //Choose initial clusters (means)
    std::uniform_int_distribution<> uniformDistrib(0, histogramSize - 1);
    _components[0]._mean = _histogram[uniformDistrib(*gen.get())]._value;

    std::vector<int> intervals(2 * histogramSize);
    std::vector<int> weights(intervals.size() - 1, 0);
    for (int b = 0; b < histogramSize; b++)
    {
        intervals[2 * b] = _histogram[b]._value;
        intervals[2 * b + 1] = _histogram[b]._value;
        weights[2 * b] = MathUtils::IntMax;
    }

    for (int c = 1; c < nbComponents; c++)
    {
        for (int b = 0; b < histogramSize; b++)
        {
            int sqDistance = ((int)_components[c - 1]._mean - _histogram[b]._value) * ((int)_components[c - 1]._mean - _histogram[b]._value);
            if (sqDistance < weights[2 * b])
                weights[2 * b] = sqDistance;
        }

        std::piecewise_constant_distribution<> pieceConstDistrib(intervals.begin(), intervals.end(), weights.begin());
        _components[c]._mean = (double)pieceConstDistrib(*gen.get());
    }

    //Iteration step
    std::vector<ClusterData> clusters(nbComponents);
    std::vector<int> assignedCluster(histogramSize, -1);
    double meanMaxDiff = MathUtils::DoubleMax;
    int iteration = 0;

    while (meanMaxDiff > _convergenceTol && iteration < _nbIter)
    {
        for (ClusterData& cluster : clusters)
        {
            cluster._sum = 0;
            cluster._count = 0;
        }

        //Assign each point to closest cluster centroid
        for (int b= 0; b < histogramSize; b++)
        {
            double distanceMin = MathUtils::DoubleMax;
            for (int c = 0; c < nbComponents; c++)
            {
                double distance = abs(_components[c]._mean - (double)_histogram[b]._value);
                if (distance < distanceMin)
                {
                    assignedCluster[b] = c;
                    distanceMin = distance;
                }
            }
            clusters[assignedCluster[b]]._sum += _histogram[b]._value * _histogram[b]._count;
            clusters[assignedCluster[b]]._count += _histogram[b]._count;
        }

        //Compute new centroids
        meanMaxDiff = 0;
        for (int c = 0; c < nbComponents; c++)
        {
            double newCentroid = (double)clusters[c]._sum / (double)clusters[c]._count;
            double meanDiff = abs(_components[c]._mean - newCentroid);
            if (meanDiff > meanMaxDiff)
                meanMaxDiff = meanDiff;

            _components[c]._mean = newCentroid;
        }

        iteration++;
    }

    //Initialize variances and weights
    for (int b = 0; b < histogramSize; b++)
    {
        const int cluster = assignedCluster[b];
        _components[cluster]._variance += ((double)_histogram[b]._value - _components[cluster]._mean) * ((double)_histogram[b]._value - _components[cluster]._mean) * (double)_histogram[b]._count;
    }
    for (int c = 0; c < nbComponents; c++)
    {
        _components[c]._variance = _components[c]._variance / (double)clusters[c]._count;
        if (_components[c]._variance < EpsilonCovariance)
            _components[c]._variance = EpsilonCovariance;
        _components[c]._weight = 1. / (double)nbComponents;
    }

}

double GaussianMixtureModel::runExpectationMaximization(int histogramSize, int nbComponents)
{
    std::vector<double> evals(nbComponents * histogramSize); //Gaussian PDF evaluations
    std::vector<double> resps(nbComponents * histogramSize); //Responsabilities
    std::vector<double> composResp(nbComponents);
    int iteration = 0;
    evalGaussianPDF(evals, histogramSize, nbComponents);
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
                resps[r] = (evals[r] * (double)_histogram[b]._count) / respAcc;
            }
        }

        //Maximization step
        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._mean = 0;
            _components[c]._variance = 0;
            composResp[c] = 0;
        }

        for (int b = 0, r = 0; b < histogramSize; b++)
        {
            for (int c = 0; c < nbComponents; c++, r++)
            {
                composResp[c] += resps[r];
                _components[c]._mean += resps[r] * (double)_histogram[b]._value;
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
                _components[c]._variance += resps[r] * ((double)_histogram[b]._value - _components[c]._mean) * ((double)_histogram[b]._value - _components[c]._mean);
            }

        }

        for (int c = 0; c < nbComponents; c++)
        {
            _components[c]._variance /= composResp[c];
            if (_components[c]._variance < EpsilonCovariance)
                _components[c]._variance = EpsilonCovariance;

            _components[c]._weight = composResp[c] / _nbData;
        }
    
        //Compute log-likelihood
        evalGaussianPDF(evals, histogramSize, nbComponents);
        double newLogLH = logLikelihood(evals, histogramSize, nbComponents);
        logLHDiff = newLogLH - logLH;
        logLH = newLogLH;

        iteration++;
    }

    return logLH;
}

void GaussianMixtureModel::evalGaussianPDF(std::vector<double>& evals, int histogramSize, int nbComponents) const
{
    std::vector<double> coeffs(3 * nbComponents);
    for (int c = 0, k = 0; c < nbComponents; c++, k += 3)
    {
        coeffs[k] = _components[c]._mean;
        coeffs[k + 1] = 2. * _components[c]._variance;
        coeffs[k + 2] = -0.5 * log(2. * std::numbers::pi * _components[c]._variance) + log(_components[c]._weight);

    }
    for (int b = 0, r = 0; b < histogramSize; b++)
    {
        for (int c = 0, k = 0; c < nbComponents; c++, k += 3, r++)
        {
            evals[r] = exp(- (_histogram[b]._value - coeffs[k]) * (_histogram[b]._value - coeffs[k]) / coeffs[k + 1] + coeffs[k + 2]);
            if (evals[r] < MathUtils::DoubleEpsilon)
                evals[r] = MathUtils::DoubleEpsilon;
        }
    }
}

double GaussianMixtureModel::logLikelihood(const std::vector<double>& evals, int histogramSize, int nbComponents) const
{
    double logLH = 0;
    for (int b = 0, r = 0; b < histogramSize; b++)
    {
        double binEval = 0;
        for (int c = 0; c < nbComponents; c++, r++)
        {
            binEval += evals[r];
        }
        logLH += log(binEval) * (double)_histogram[b]._count;
    }

    return logLH;
}

void GaussianMixtureModel::computeBIC(double logLH, int nbComponents)
{
	_BIC = -2. * logLH + (double)(3 * nbComponents - 1) * log(_nbData);
}

