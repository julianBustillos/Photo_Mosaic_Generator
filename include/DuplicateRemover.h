#pragma once

#include "Tiles.h"


class DuplicateRemover
{
private:
    static constexpr double DistanceTol = 0.16;

public:
    void run(Tiles& tiles) const;
};

