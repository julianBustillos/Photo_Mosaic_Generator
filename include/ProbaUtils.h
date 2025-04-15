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

    //double normalPDF();
    //double gmmPDF();

    //TODO ADD CDF STUFF
    // dataHistogram
    // dataCDF
    // gmmCDF
    //wassersteinDistance

};

