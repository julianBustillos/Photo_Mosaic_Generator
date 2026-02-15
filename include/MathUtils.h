#pragma once

#include <limits>
#include <Eigen/Dense>
#include <Eigen/Cholesky>


namespace MathUtils
{
    const double DoubleMax = std::numeric_limits<double>::max();
    const double DoubleEpsilon = std::numeric_limits<double>::epsilon();
    const int IntMax = std::numeric_limits<int>::max();

    template <unsigned int N>
    using VectorNd = Eigen::Matrix<double, N, 1>;

    template <unsigned int N>
    using MatrixNd = Eigen::Matrix<double, N, N>;

    template <unsigned int N>
    double det(const MatrixNd<N>& M)
    {
        Eigen::JacobiSVD<MatrixNd<N>> svdM(M, Eigen::ComputeFullV | Eigen::ComputeFullU);

        VectorNd<3> sValues = svdM.singularValues();
        double determinant = 1.;
        for (int i = 0; i < N; i++)
            if (abs(sValues(i)) > DoubleEpsilon)
                determinant *= sValues(i);

        return determinant;
    }

    template <unsigned int N>
    MatrixNd<N> inv(const MatrixNd<N>& M)
    {
        Eigen::JacobiSVD<MatrixNd<N>> svdM(M, Eigen::ComputeFullV | Eigen::ComputeFullU);

        MatrixNd<N> invS = svdM.singularValues().asDiagonal();
        for (int i = 0; i < N; i++)
            if (abs(invS(i, i)) > DoubleEpsilon)
                invS(i, i) = 1 / invS(i, i);

        return svdM.matrixV() * invS * svdM.matrixU().transpose();
    }

    template <unsigned int N>
    MatrixNd<N> sqrt(const MatrixNd<N>& M)
    {
        Eigen::JacobiSVD<MatrixNd<N>> svdM(M, Eigen::ComputeFullV | Eigen::ComputeFullU);

        MatrixNd<N> sqrtS = svdM.singularValues().asDiagonal();
        for (int i = 0; i < N; i++)
            sqrtS(i, i) = std::sqrt(sqrtS(i, i));

        return svdM.matrixU() * sqrtS * svdM.matrixV().transpose();
    }

    template <unsigned int N>
    MatrixNd<N> Cholesky(const MatrixNd<N>& M)
    {
        Eigen::LLT<MatrixNd<N>> llt(M);
        return llt.matrixL();
    }
};



