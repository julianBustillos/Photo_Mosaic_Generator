#pragma once

#include "ITilesCleaner.h"


class TilesCleanerImpl : public ITilesCleaner
{
private:
    static constexpr double DistanceTolerance = 0.16;

public:
    virtual void clean(ITiles& tiles) const;
};

