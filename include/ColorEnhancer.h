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
    static constexpr double GSSTolerance = 1e-3;
    static constexpr int CoverageGridDivisions = 32;
    static constexpr double CoverageConfidence = 0.95;
    static constexpr double CoverageMinRatio = 0.1;

public:
    ColorEnhancer(const ProbaUtils::Histogram<3>& sourceHistogram, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm, const ProbaUtils::GMMSampleDatas<3>& datas);
    ~ColorEnhancer();

public:
    void apply(cv::Mat& enhancedImage, double blending);

private:
    double computeGMMSamplingCoverage(const ProbaUtils::GMMNDComponents<3>& gmm, int nbDivisions, double confidence);
    double computeCoverageDist(double t, double coverage);
    double goldenSectionSearch(double coverage);
    void computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double t) const;

private:
    const ProbaUtils::Histogram<3>& _sourceHistogram;
    const ProbaUtils::GMMNDComponents<3>& _sourceGmm;
    const ProbaUtils::GMMNDComponents<3>& _targetGmm;
    const ProbaUtils::GMMSampleDatas<3>& _datas;
    std::vector<double> _histCompProbas;
    ProbaUtils::W2Minimizers _wstar;
    double _blendingScale;
};