#pragma once

#include "MathUtils.h"
#include <opencv2/opencv.hpp>
#include "unsupported/Eigen/MatrixFunctions"
#include "network_simplex_simple.h"
#include <numbers>
#include <vector>


namespace ProbaUtils
{
    template <unsigned int N>
    struct Bin
    {
        MathUtils::VectorNd<N> _value;
        int _count;
    };

    template <unsigned int N>
    struct SampleData
    {
        std::vector<unsigned int> _mapId;
        std::vector<Bin<N>> _histogram;
        int _nbData;
    };

    template <unsigned int N>
    struct GaussianComponent
    {
        MathUtils::VectorNd<N> _mean;
        MathUtils::MatrixNd<N> _covariance;
        double _weight;
    };

    struct W2Coefficient
    {
        double _value;
        unsigned int _k;
        unsigned int _l;
    };

    using W2Minimizers = std::vector<W2Coefficient>;

    template <unsigned int N>
    using GMMNDComponents = std::vector<GaussianComponent<N>>;

    template <unsigned int N>
    void computeSampleData(SampleData<N>& sampleData, const double* data, int nbData);

    template <unsigned int N>
    void evalGaussianPDF(std::vector<double>& densities, std::vector<double>& norms, const std::vector<Bin<N>>& histogram, const GMMNDComponents<N>& gmm, bool normalizeDensities);

    template <unsigned int N>
    double computeGW2(const GaussianComponent<N>& gaussian0, const GaussianComponent<N>& gaussian1); //Wasserstein-2 distance between 2 Nd gaussians.

    template <unsigned int N>
    double computeGmmW2(W2Minimizers& wstar, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1); //Wasserstein-2 distance and coefficients between 2 Nd gmms.

    template <unsigned int N>
    void computeGmmInterp(GMMNDComponents<N>& gmmt, double t, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1, const W2Minimizers& wstar); //Gaussian interpolation between two Nd gmms.

    template <unsigned int N>
    double computeHistCoverage(const GMMNDComponents<N>&gmm, const std::vector<Bin<N>>& histogram, double minDensity);
};


template<unsigned int N>
void ProbaUtils::computeSampleData(SampleData<N>& sampleData, const double* data, int nbData)
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

    sampleData._mapId.reserve(nbData);
    std::map<MathUtils::VectorNd<N>, BinData, decltype(cmp)> histDataMap(cmp);
    for (int i = 0; i < nbData; i++)
    {
        auto& histData = histDataMap[MathUtils::VectorNd<N>(&data[i * N])];
        histData._count++;
        if (histData._index < 0)
            histData._index = histDataMap.size() - 1;
        sampleData._mapId.emplace_back(histData._index);
    }

    sampleData._histogram.resize(histDataMap.size());
    for (auto& data : histDataMap)
    {
        auto& bin = sampleData._histogram[data.second._index];
        bin._value = data.first;
        bin._count = data.second._count;
    }
    sampleData._nbData = nbData;
}

template <unsigned int N>
void ProbaUtils::evalGaussianPDF(std::vector<double>& densities, std::vector<double>& norms, const std::vector<Bin<N>>& histogram, const GMMNDComponents<N>& gmm, bool normalizeDensities)
{
    const int histogramSize = histogram.size();
    const int nbComponents = gmm.size();
    std::vector<MathUtils::MatrixNd<N>> halfCovInv(nbComponents);
    std::vector<double> constLog(nbComponents);
    for (int c = 0; c < nbComponents; c++)
    {
        halfCovInv[c] = 0.5 * MathUtils::inv<3>(gmm[c]._covariance);
        constLog[c] = -0.5 * N * log(2. * std::numbers::pi) - 0.5 * log(gmm[c]._covariance.determinant()) + log(gmm[c]._weight);
    }

    MathUtils::VectorNd<N> valMeanDiff;
    for (int b = 0, e = 0; b < histogramSize; b++)
    {
        norms[b] = 0;
        int zeroCount = 0;
        for (int c = 0; c < nbComponents; c++, e++)
        {
            valMeanDiff = histogram[b]._value - gmm[c]._mean;
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
                zeroCount++;
            }
            norms[b] += densities[e];
        }

        if (zeroCount == nbComponents)
        {
            e -= nbComponents;
            for (int c = 0; c < nbComponents; c++, e++)
                densities[e] = MathUtils::DoubleEpsilon;
            norms[b] = nbComponents * MathUtils::DoubleEpsilon;
        }
    }

    if (normalizeDensities)
    {
        for (int b = 0, e = 0; b < histogramSize; b++)
            for (int c = 0; c < nbComponents; c++, e++)
                densities[e] /= norms[b];
    }
}

template<unsigned int N>
double ProbaUtils::computeGW2(const GaussianComponent<N>& gaussian0, const GaussianComponent<N>& gaussian1)
{
    MathUtils::VectorNd<N> gaussianDiff = gaussian1._mean - gaussian0._mean;
    MathUtils::MatrixNd<N> cov0Sqrt = MathUtils::sqrt<3>(gaussian0._covariance);

    return gaussianDiff.dot(gaussianDiff) + (gaussian0._covariance + gaussian1._covariance - 2. * MathUtils::sqrt<3>(cov0Sqrt * gaussian1._covariance * cov0Sqrt)).trace();
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
void ProbaUtils::computeGmmInterp(GMMNDComponents<N>& gmmt, double t, const GMMNDComponents<N>& gmm0, const GMMNDComponents<N>& gmm1, const W2Minimizers& wstar)
{
    const int nbComponents = wstar.size();
    gmmt.resize(nbComponents);

    std::vector<MathUtils::VectorNd<3>> meanT(nbComponents);

    for (int c = 0; c < nbComponents; c++)
    {
        double k = wstar[c]._k;
        double l = wstar[c]._l;

        gmmt[c]._mean = (1. - t) * gmm0[k]._mean + t * gmm1[l]._mean;
        const MathUtils::MatrixNd<3>& sigma0 = gmm0[k]._covariance;
        const MathUtils::MatrixNd<3>& sigma1 = gmm1[l]._covariance;
        const MathUtils::MatrixNd<3> sigma1Sqrt = MathUtils::sqrt<3>(sigma1);
        const MathUtils::MatrixNd<3> C = sigma1Sqrt * MathUtils::sqrt<3>(MathUtils::inv<3>(sigma1Sqrt * sigma0 * sigma1Sqrt)) * sigma1Sqrt;
        const MathUtils::MatrixNd<3> Cinterp = (1 - t) * MathUtils::MatrixNd<3>::Identity() + t * C;
        gmmt[c]._covariance = Cinterp * sigma0 * Cinterp;
        gmmt[c]._weight = wstar[c]._value;
    }
}

template<unsigned int N>
double ProbaUtils::computeHistCoverage(const GMMNDComponents<N>& gmm, const std::vector<Bin<N>>& histogram, double minDensity)
{
    int histogramSize = histogram.size();
    int nbComponents = gmm.size();
    std::vector<double> densities(nbComponents * histogramSize);
    std::vector<double> norms(histogramSize);
    ProbaUtils::evalGaussianPDF(densities, norms, histogram, gmm, false);

    int countValidDensities = 0;
    for (int b = 0, e = 0; b < histogramSize; b++)
    {
        double binDensity = 0;
        for (int c = 0; c < nbComponents; c++, e++)
            binDensity += densities[e];

        if (binDensity > minDensity)
            countValidDensities += histogram[b]._count;
    }

    return (double)countValidDensities / (double)histogramSize;
}

