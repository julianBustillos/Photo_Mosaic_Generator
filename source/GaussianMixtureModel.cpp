#include "GaussianMixtureModel.h"
#include <random>
#include <map>
#include <set>
#include <limits>
#include <numbers>


const double GaussianMixtureModel::EpsilonCovariance = 1. / 16.;

double GaussianMixtureModel::square(double value)
{
    return value * value;
}


std::vector<GaussianMixtureModel::Component> GaussianMixtureModel::findOptimalComponents(const std::vector<int>& data, int maxNbComponents, double kmeansTol, int kmeansIter, double emTol, int emIter, bool defaultSeed)
{
    GaussianMixtureModel gmm(kmeansTol, kmeansIter, emTol, emIter, defaultSeed);
    gmm.setData(data);
    std::vector<Component> optimalComponents;
    double optimalBIC = std::numeric_limits<double>::max();

    for (int nbComponents = 1; nbComponents <= maxNbComponents; nbComponents++)
    {
        gmm.run(nbComponents);
        double BIC = gmm.getBIC();
        if (BIC < optimalBIC)
        {
            optimalComponents = gmm.getComponents();
            optimalBIC = BIC;
        }
    }

    return optimalComponents;
}

GaussianMixtureModel::GaussianMixtureModel(double kmeansTol, int kmeansIter, double emTol, int emIter, bool defaultSeed) :
    _kmeansTol(kmeansTol), _kmeansIter(kmeansIter), _emTol(emTol), _emIter(emIter), _defaultSeed(defaultSeed), _nbData(0), _BIC(-1)
{
}

void GaussianMixtureModel::setData(const std::vector<int>& data)
{
	std::map<int, int> valueCount;
	for (int value : data)
		valueCount[value]++;

	_histogram.reserve(valueCount.size());
	for (auto element : valueCount)
		_histogram.emplace_back(element.first, element.second);

    _nbData = data.size();
    _components.clear();
}

bool GaussianMixtureModel::run(int nbComponents)
{
    if (!checkValidity(nbComponents))
        return false;

    _components.resize(nbComponents);

    runKmeansPlusPlus();
    runExpectationMaximization();
    computeBIC();
    
    return true;
}

double GaussianMixtureModel::getBIC()
{
    return _BIC;
}

std::vector<GaussianMixtureModel::Component> GaussianMixtureModel::getComponents()
{
    return _components;
}

bool GaussianMixtureModel::checkValidity(int nbComponents)
{
    return nbComponents > 0 && _histogram.size() >= nbComponents;
}

void GaussianMixtureModel::runKmeansPlusPlus()
{
    //Use kmeans++ as initializer for GMM
    std::random_device rd;
    std::unique_ptr<std::mt19937> gen = _defaultSeed ? std::make_unique<std::mt19937>() : std::make_unique<std::mt19937>(rd());

    //Choose initial clusters (means)
    std::uniform_int_distribution<> uniformDistrib(0, _histogram.size() - 1);
    _components[0]._mean = _histogram[uniformDistrib(*gen.get())]._value;

    std::vector<int> intervals(2 * _histogram.size());
    std::vector<int> weights(intervals.size() - 1, 0);
    for (int b = 0; b < _histogram.size(); b++)
    {
        intervals[2 * b] = _histogram[b]._value;
        intervals[2 * b + 1] = _histogram[b]._value;
        weights[2 * b] = std::numeric_limits<int>::max();
    }

    for (int c = 1; c < _components.size(); c++)
    {
        for (int b = 0; b < _histogram.size(); b++)
        {
            int sqDistance = square((int)_components[c - 1]._mean - _histogram[b]._value);
            if (sqDistance < weights[2 * b])
                weights[2 * b] = sqDistance;
        }

        std::piecewise_constant_distribution<> pieceConstDistrib(intervals.begin(), intervals.end(), weights.begin());
        _components[c]._mean = (double)pieceConstDistrib(*gen.get());
    }

    //Iteration step
    std::vector<ClusterData> clusters(_components.size());
    std::vector<int> assignedCluster(_histogram.size(), -1);
    double meanMaxDiff = std::numeric_limits<double>::max();
    int iteration = 0;

    while (meanMaxDiff > _kmeansTol && iteration < _kmeansIter)
    {
        for (ClusterData& cluster : clusters)
        {
            cluster._sum = 0;
            cluster._count = 0;
        }

        //Assign each point to closest cluster centroid
        for (int b= 0; b < _histogram.size(); b++)
        {
            double distanceMin = std::numeric_limits<double>::max();
            for (int c = 0; c < _components.size(); c++)
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
        for (int c = 0; c < _components.size(); c++)
        {
            double newCentroid = (double)clusters[c]._sum / (double)clusters[c]._count;
            double meanDiff = abs(_components[c]._mean - newCentroid);
            if (meanDiff > meanMaxDiff)
                meanMaxDiff = meanDiff;

            _components[c]._mean = newCentroid;
        }

        iteration++;
    }

    //Initialize covariances and weights
    for (int b = 0; b < _histogram.size(); b++)
    {
        const int cluster = assignedCluster[b];
        _components[cluster]._covariance += square((double)_histogram[b]._value - _components[cluster]._mean) * (double)_histogram[b]._count;
    }
    for (int c = 0; c < _components.size(); c++)
    {
        _components[c]._covariance = _components[c]._covariance / (double)clusters[c]._count;
        if (_components[c]._covariance < EpsilonCovariance)
            _components[c]._covariance = EpsilonCovariance;
        _components[c]._weight = 1. / (double)_components.size();
    }

}

void GaussianMixtureModel::runExpectationMaximization()
{
    std::vector<std::vector<double>> resp(_components.size(), std::vector<double>(_histogram.size())); //Responsabilities
    int iteration = 0;
    double logLH = logLikelihood();
    double logLHDiff = std::numeric_limits<double>::max(); //Log-likelihood iteration difference

    while (logLHDiff > _emTol && iteration < _emIter)
    {
        //Expectation step
        std::vector<double> respAcc(_histogram.size(), 0);
        for (int c = 0; c < _components.size(); c++)
        {
            for (int b = 0; b < _histogram.size(); b++)
            {
				resp[c][b] = normalPDF((double)_histogram[b]._value, _components[c]);
                if (resp[c][b] > std::numeric_limits<double>::epsilon())
                    respAcc[b] += resp[c][b];
                else
                    resp[c][b] = 0;
            }
        }
        for (int c = 0; c < _components.size(); c++)
        {
            for (int b = 0; b < _histogram.size(); b++)
            {
				if (respAcc[b] > std::numeric_limits<double>::epsilon())
					resp[c][b] /= respAcc[b];
				else
					resp[c][b] = 1 / (double)_components.size();
            }
        }

        //Maximization step
        for (int c = 0; c < _components.size(); c++)
        {
            double clusterResp = 0;
            _components[c]._mean = 0;
            for (int b = 0; b < _histogram.size(); b++)
            {
                clusterResp += resp[c][b] * (double)_histogram[b]._count;
                _components[c]._mean += resp[c][b] * (double)_histogram[b]._value * (double)_histogram[b]._count;
            }
            _components[c]._mean /= clusterResp;

            _components[c]._covariance = 0;
            for (int b = 0; b < _histogram.size(); b++)
            {
                _components[c]._covariance += resp[c][b] * square((double)_histogram[b]._value - _components[c]._mean) * (double)_histogram[b]._count;
            }
            _components[c]._covariance /= clusterResp;
            if (_components[c]._covariance < EpsilonCovariance)
                _components[c]._covariance = EpsilonCovariance;

            _components[c]._weight = clusterResp / _nbData;
        }
    
        //Compute log-likelihood
        double newLogLH = logLikelihood();
        logLHDiff = newLogLH - logLH;
        logLH = newLogLH;

        iteration++;
    }
}

double GaussianMixtureModel::normalPDF(double x, const Component& component)
{
    return exp(-square(x - component._mean) / (2. * component._covariance)) / sqrt(2. * std::numbers::pi * component._covariance) * component._weight;
}

double GaussianMixtureModel::logLikelihood()
{
    double logLH = 0;
    for (int b = 0; b < _histogram.size(); b++)
    {
        double valueLogLH = 0;
        for (int c = 0; c < _components.size(); c++)
        {
            valueLogLH += normalPDF((double)_histogram[b]._value, _components[c]);
        }
        logLH += log(valueLogLH) * (double)_histogram[b]._count;
    }
    return logLH;
}

void GaussianMixtureModel::computeBIC()
{
	_BIC = -2. * logLikelihood() + (double)(3 * _components.size() - 1) * log(_nbData);
}

