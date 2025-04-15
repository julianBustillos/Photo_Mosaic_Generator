#pragma once

#include "Tiles.h"


class DuplicateRemover
{
private:
    static constexpr double DistanceTolerance = 0.16;

public:
    void run(Tiles& tiles) const;
};

