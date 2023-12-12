#pragma once

#include "Parameters.h"
#include "Photo.h"
#include "IPixelAdapter.h"
#include "IRegionOfInterest.h"
#include "ITiles.h"
#include "IMatchSolver.h"
#include "MosaicBuilder.h"


class MosaicGenerator
{
public:
    MosaicGenerator(const Parameters& parameters);
    ~MosaicGenerator();
    void Build();

private:
    const Photo _photo;
    IPixelAdapter* _pixelAdapter;
    IRegionOfInterest* _roi;
    ITiles* _tiles;
    IMatchSolver* _matchSolver;
    MosaicBuilder _mosaicBuilder;
};