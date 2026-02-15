#pragma once

#include "MathUtils.h"
#include <opencv2/opencv.hpp>
#include "network_simplex_simple.h"
#include <numbers>
#include <vector>
#include <random>


namespace ProbaUtils
{
    template <unsigned int N>
    struct Histogram
    {
        std::vector<unsigned int> _mapId;
        std::vector<MathUtils::VectorNd<N>> _values;
        std::vector<int> _counts;
        int _nbData;
    };

    template <unsigned int N>
    struct GaussianComponent
    {
        MathUtils::VectorNd<N> _mean;
        MathUtils::MatrixNd<N> _covariance;
        double _weight;
    };

    template <unsigned int N>
    using GMMNDComponents = std::vector<GaussianComponent<N>>;

    struct W2Coefficient
    {
        double _value;
        unsigned int _k;
        unsigned int _l;
    };

    using W2Minimizers = std::vector<W2Coefficient>;

    template <unsigned int N>
    struct GMMSamplerData
    {
        double _component;
        MathUtils::VectorNd<N> _stdGaussian;
    };

    template <unsigned int N>
    using GMMSamplerDatas = std::vector<GMMSamplerData<N>>;

    template <unsigned int N>
    using GMMSamples = std::vector<MathUtils::VectorNd<N>>;


    template <unsigned int N>
    void computeHistogram(Histogram<N>& histogram, const double* data, int nbData);

    template <unsigned int N>
    void evalGaussianPDF(std::vector<double>& densities, std::vector<double>& norms, const std::vector<MathUtils::VectorNd<N>>& values, const GMMNDComponents<N>& gmm, bool normalizeDensities);

    template <unsigned int N>
    double computeGW2(const GaussianComponent<N>& gaussian0, const GaussianComponent<N>& gaussian1); //Wasserstein-2 distance between 2 Nd gaussians.

    template <unsigned int N>
    double computeGmmW2(W2Minimizers& wstar, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1); //Wasserstein-2 distance and coefficients between 2 Nd gmms.

    template <unsigned int N>
    void computeGmmInterpolation(GMMNDComponents<N>& gmmt, double t, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1, const W2Minimizers& wstar); //Gaussian interpolation between two Nd gmms.

    template <unsigned int N>
    void generateGMMSamplerDatas(GMMSamplerDatas<N>& datas, int nbSamples, bool defaultSeed);

    template <unsigned int N>
    void computeGmmSamples(GMMSamples<N>& samples, const GMMNDComponents<N>& gmm, const GMMSamplerDatas<N>& datas);
};


template<unsigned int N>
void ProbaUtils::computeHistogram(Histogram<N>& histogram, const double* data, int nbData)
{
    auto cmp = [](const MathUtils::VectorNd<N>& value0, const MathUtils::VectorNd<N>& value1)
        {
            for (int n = 0; n < N; n++)
            {
                if (value0(n, 0) != value1(n, 0))
                    return value0(n, 0) < value1(n, 0);
            }
            return false;
        };

    struct BinData
    {
        unsigned int _count = 0;
        int _index = -1;
    };

    histogram._mapId.reserve(nbData);
    std::map<MathUtils::VectorNd<N>, BinData, decltype(cmp)> histDataMap(cmp);
    for (int i = 0; i < nbData; i++)
    {
        auto& histData = histDataMap[MathUtils::VectorNd<N>(&data[i * N])];
        histData._count++;
        if (histData._index < 0)
            histData._index = histDataMap.size() - 1;
        histogram._mapId.emplace_back(histData._index);
    }

    histogram._values.resize(histDataMap.size());
    histogram._counts.resize(histDataMap.size());
    for (auto& data : histDataMap)
    {
        histogram._values[data.second._index] = data.first;
        histogram._counts[data.second._index] = data.second._count;
    }
    histogram._nbData = nbData;
}

template <unsigned int N>
void ProbaUtils::evalGaussianPDF(std::vector<double>& densities, std::vector<double>& norms, const std::vector<MathUtils::VectorNd<N>>& values, const GMMNDComponents<N>& gmm, bool normalizeDensities)
{
    const int nbValues = values.size();
    const int nbComponents = gmm.size();
    std::vector<MathUtils::MatrixNd<N>> halfCovInv(nbComponents);
    std::vector<double> constLog(nbComponents);
    for (int c = 0; c < nbComponents; c++)
    {
        halfCovInv[c] = 0.5 * MathUtils::inv<N>(gmm[c]._covariance);
        constLog[c] = -0.5 * N * log(2. * std::numbers::pi) - 0.5 * log(MathUtils::det<N>(gmm[c]._covariance)) + log(gmm[c]._weight);
    }

    MathUtils::VectorNd<N> valMeanDiff;
    for (int b = 0, e = 0; b < nbValues; b++)
    {
        norms[b] = 0;
        for (int c = 0; c < nbComponents; c++, e++)
        {
            valMeanDiff = values[b] - gmm[c]._mean;
            double value = constLog[c];
            for (int i = 0, k = 0; i < N; i++, k+= i)
            {
                value -= halfCovInv[c].data()[k] * valMeanDiff.data()[i] * valMeanDiff.data()[i];
                k++;
                for (int j = i + 1; j < N; j++, k++)
                    value -= 2. * halfCovInv[c].data()[k] * valMeanDiff.data()[i] * valMeanDiff.data()[j];
            }
            densities[e] = exp(value);
            if (densities[e] < MathUtils::DoubleEpsilon)
            {
                densities[e] = 0;
            }
            norms[b] += densities[e];
        }

        if (norms[b] == 0)
            norms[b] = MathUtils::DoubleEpsilon;
    }

    if (normalizeDensities)
    {
        for (int b = 0, e = 0; b < nbValues; b++)
            for (int c = 0; c < nbComponents; c++, e++)
                densities[e] /= norms[b];
    }
}

template<unsigned int N>
double ProbaUtils::computeGW2(const GaussianComponent<N>& gaussian0, const GaussianComponent<N>& gaussian1)
{
    MathUtils::VectorNd<N> gaussianDiff = gaussian1._mean - gaussian0._mean;
    MathUtils::MatrixNd<N> cov0Sqrt = MathUtils::sqrt<N>(gaussian0._covariance);

    return gaussianDiff.dot(gaussianDiff) + (gaussian0._covariance + gaussian1._covariance - 2. * MathUtils::sqrt<N>(cov0Sqrt * gaussian1._covariance * cov0Sqrt)).trace();
}

template<unsigned int N>
double ProbaUtils::computeGmmW2(W2Minimizers& wstar, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1)
{
    unsigned int K0 = gmm0.size();
    unsigned int K1 = gmm1.size();
    std::vector<double> weights0(K0);
    std::vector<double> weights1(K1);

    lemon::FullBipartiteDigraph digraph(K0, K1);
    lemon::NetworkSimplexSimple<lemon::FullBipartiteDigraph, double, double, int> network(digraph, false, K0 + K1, K0 * K1);

    for (int k = 0; k < K0; k++)
        weights0[k] = gmm0[k]._weight;

    for (int l = 0; l < K1; l++)
        weights1[l] = -gmm1[l]._weight;

    network.supplyMap(weights0.data(), K0, weights1.data(), K1);

    for (int k = 0, i = 0; k < K0; k++)
        for (int l = 0; l < K1; l++, i++)
            network.setCost(i, computeGW2(gmm0[k], gmm1[l]));

    int ret = network.run();
    double distance = network.totalCost();

    wstar.reserve(K0 + K1 - 1);
    for (int k = 0, i = 0; k < K0; k++)
    {
        for (int l = 0; l < K1; l++, i++)
        {
            double flow = network.flow(i);
            if (abs(flow) > MathUtils::DoubleEpsilon)
                wstar.emplace_back(flow, k, l);
        }
    }

    return distance;
}

template<unsigned int N>
void ProbaUtils::computeGmmInterpolation(GMMNDComponents<N>& gmmt, double t, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1, const W2Minimizers& wstar)
{
    const int nbComponents = wstar.size();
    gmmt.resize(nbComponents);

    std::vector<MathUtils::VectorNd<N>> meanT(nbComponents);

    for (int c = 0; c < nbComponents; c++)
    {
        double k = wstar[c]._k;
        double l = wstar[c]._l;

        gmmt[c]._mean = (1. - t) * gmm0[k]._mean + t * gmm1[l]._mean;
        const MathUtils::MatrixNd<N>& sigma0 = gmm0[k]._covariance;
        const MathUtils::MatrixNd<N>& sigma1 = gmm1[l]._covariance;
        const MathUtils::MatrixNd<N> sigma1Sqrt = MathUtils::sqrt<N>(sigma1);
        const MathUtils::MatrixNd<N> C = sigma1Sqrt * MathUtils::sqrt<N>(MathUtils::inv<N>(sigma1Sqrt * sigma0 * sigma1Sqrt)) * sigma1Sqrt;
        const MathUtils::MatrixNd<N> Cinterp = (1 - t) * MathUtils::MatrixNd<N>::Identity() + t * C;
        gmmt[c]._covariance = Cinterp * sigma0 * Cinterp;
        gmmt[c]._weight = wstar[c]._value;
    }
}

template<unsigned int N>
void ProbaUtils::generateGMMSamplerDatas(GMMSamplerDatas<N>& datas, int nbSamples, bool defaultSeed)
{
    std::unique_ptr<std::mt19937> gen = defaultSeed ? std::make_unique<std::mt19937>() : std::make_unique<std::mt19937>(std::random_device{}());
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, 1.0);
    datas.resize(nbSamples);

    for (int s = 0; s < nbSamples; s++)
    {
        datas[s]._component = uniform(*gen.get());
        for (int n = 0; n < N; n++)
            datas[s]._stdGaussian(n, 0) = normal(*gen.get());
    }

    std::sort(datas.begin(), datas.end(), [](const GMMSamplerData<N> lhs, const GMMSamplerData<N> rhs) { return lhs._component < rhs._component; });
}

template<unsigned int N>
void ProbaUtils::computeGmmSamples(GMMSamples<N>& samples, const GMMNDComponents<N>& gmm, const GMMSamplerDatas<N>& datas)
{
    const int nbComponents = gmm.size();
    std::vector<double> cumulatedWeights(nbComponents);
    cumulatedWeights[0] = gmm[0]._weight;
    cumulatedWeights[nbComponents - 1] = 1.0;
    for (int w = 1; w < nbComponents - 1; w++)
        cumulatedWeights[w] = cumulatedWeights[w - 1] + gmm[w]._weight;

    const int nbSamples = datas.size();
    int currComponent = 0;
    bool updateComponent = true;
    MathUtils::MatrixNd<N> currCovCholesky;
    samples.resize(nbSamples);

    for (int s = 0; s < nbSamples; s++)
    {
        if (datas[s]._component > cumulatedWeights[currComponent])
        {
            for (currComponent++; currComponent < nbComponents; currComponent++)
                if (datas[s]._component <= cumulatedWeights[currComponent])
                    break;
            updateComponent = true;
        }

        if (updateComponent)
        {
            currCovCholesky = MathUtils::Cholesky(gmm[currComponent]._covariance);
            updateComponent = false;
        }

        samples[s] = currCovCholesky.triangularView<Eigen::Lower>() * datas[s]._stdGaussian + gmm[currComponent]._mean;
    }
}

