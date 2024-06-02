#pragma once

#include "ITilesCleaner.h"


class TilesCleanerImpl : public ITilesCleaner
{
private:
    static constexpr double DistanceTolerance = 0.16;

public:
    virtual void clean(ITiles& tiles) const;

private:
    void cleanBlurries(ITiles& tiles) const;
    void cleanDuplicates(ITiles& tiles) const;
    bool isBlurry(const cv::Mat& image) const;
};

