#pragma once

#include "Photo.h"
#include "MathUtils.h"
#include "ProbaUtils.h"
#include "GaussianMixtureModel.h"
#include <tuple>
#include <unordered_map>


class ColorEnhancer
{
private:
    static constexpr int FilterRadius = 4;
    static constexpr double FilterEpsilon = (0.01 * 255) * (0.01 * 255);
    static constexpr double CoverageMinDensity = 1e-12;
    static constexpr double CoverageMinRatio = 0.1;
    static constexpr double GSSTolerance = 1e-5;
    static std::vector<ProbaUtils::Bin<3>> ColorSpace;

public:
    static void initializeColorSpace(double valueMin, double valueMax, int nbElements, int nbDivisions);

public:
    ColorEnhancer(const ProbaUtils::SampleData<3>& sourceSample, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm);
    ~ColorEnhancer();

public:
    void apply(cv::Mat& enhancedImage, double blending);

private:
    double computeCoverageDist(double t, double coverage);
    double goldenSectionSearch(double coverage);
    void computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double t) const;

private:
    const ProbaUtils::SampleData<3>& _sourceSample;
    const ProbaUtils::GMMNDComponents<3>& _sourceGmm;
    const ProbaUtils::GMMNDComponents<3>& _targetGmm;
    std::vector<double> _histCompProbas;
    ProbaUtils::W2Minimizers _wstar;
    double _sourceCoverage;
    double _targetCoverage;
    std::unordered_map<double, double> _coverageCacheMap;
};