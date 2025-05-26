#pragma once

#include "Photo.h"
#include "MathUtils.h"
#include "ProbaUtils.h"
#include "GaussianMixtureModel.h"
#include <tuple>


class ColorEnhancer
{
public:
    ColorEnhancer(const ProbaUtils::SampleData<3>& sourceSample, const ProbaUtils::GMMNDComponents<3>& sourceGmm, const ProbaUtils::GMMNDComponents<3>& targetGmm);
    ~ColorEnhancer();

public:
    void apply(cv::Mat& enhancedImage, double blending) const;

private:
    void computeColorMap(std::vector<MathUtils::VectorNd<3>>& colorMap, double blending) const;

private:
    const ProbaUtils::SampleData<3>& _sourceSample;
    const ProbaUtils::GMMNDComponents<3>& _sourceGmm;
    const ProbaUtils::GMMNDComponents<3>& _targetGmm;
    std::vector<double> _histCompDensities;
    std::vector<double> _histDensity;
    ProbaUtils::W2Minimizers _wstar;
};