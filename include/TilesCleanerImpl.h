#pragma once

#include "ITilesCleaner.h"


class TilesCleanerImpl : public ITilesCleaner
{
private:
    static constexpr double BlurinessThreashold = 100.;
    static constexpr double DistanceTolerance = 0.16;

public:
    virtual void clean(ITiles& tiles) const;

private:
    bool isBlurry(const cv::Mat& image) const;
};

