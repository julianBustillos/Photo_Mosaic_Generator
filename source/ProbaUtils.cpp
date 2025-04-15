#include "ProbaUtils.h"


ProbaUtils::CDF::CDF()
{
    for (int c = 0; c < 758; c++)
        _data[c] = 0;
}

double* ProbaUtils::CDF::operator[](int color)
{
    return &_data[256 * color];
}

const double* ProbaUtils::CDF::operator[](int color) const
{
    return &_data[256 * color];
}
