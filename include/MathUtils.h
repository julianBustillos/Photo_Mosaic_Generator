#pragma once

#include <limits>
#include <Eigen/Dense>


namespace MathUtils
{
    const double DoubleMax = std::numeric_limits<double>::max();
    const double DoubleEpsilon = std::numeric_limits<double>::epsilon();
    const int IntMax = std::numeric_limits<int>::max();

    template <unsigned int N>
    using VectorNd = Eigen::Matrix<double, N, 1>;

    template <unsigned int N>
    using MatrixNd = Eigen::Matrix<double, N, N>;

    inline double pow(double value, unsigned int exponent) { return (exponent > 0) ? value * pow(value, exponent - 1) : 1.; };
};



