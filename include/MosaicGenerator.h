#pragma once

#include "Parameters.h"
#include "Photo.h"
#include "IPixelAdapter.h"
#include "IRegionOfInterest.h"
#include "ITiles.h"
#include "IMatchSolver.h"
#include "MosaicBuilder.h"
#include <memory>


class MosaicGenerator
{
public:
    MosaicGenerator(const Parameters& parameters);
    ~MosaicGenerator();
    void Build();

private:
    std::shared_ptr<const Photo> _photo;
    std::shared_ptr<IPixelAdapter> _pixelAdapter;
    std::shared_ptr<IRegionOfInterest> _roi;
    std::shared_ptr<ITiles> _tiles;
    std::shared_ptr<IMatchSolver> _matchSolver;
    std::shared_ptr<MosaicBuilder> _mosaicBuilder;
};