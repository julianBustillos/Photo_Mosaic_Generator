#pragma once

#include <opencv2/opencv.hpp>


namespace ProbaUtils
{
    class CDF
    {
    public:
        CDF();
        double* operator[](int color);
        const double* operator[](int color) const;

    private:
        double _data[768];
    };

    struct GaussianComponent
    {
        double _mean;
        double _variance;
        double _weight;
    };

    using GMMComponents = std::vector<GaussianComponent>;

    using GMMCDFComponents = GMMComponents[3];


    void imageCDF(CDF& cdf, const cv::Mat& image);
    void imageCDF(CDF& cdf, const cv::Mat& image, const cv::Rect& box);

    double evalGaussianCDF(double x, const GaussianComponent& component, double varScale);
    double evalGmmCDF(double x, const GMMComponents& components, double varScale);
    void gmmCDF(CDF& cdf, const GMMCDFComponents& components, double varScale, double startConstr[3], double endConst[3]);

    double w1Distance(const CDF& cdf0, const CDF& cdf1); //Wasserstein-1 distance
};

