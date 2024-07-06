#pragma once

#include "Tiles.h"


class TilesCleaner
{
private:
    static constexpr double DistanceTolerance = 0.16;

public:
    void clean(Tiles& tiles) const;
};

