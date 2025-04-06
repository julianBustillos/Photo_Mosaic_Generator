#pragma once

#include "Parameters.h"
#include "Photo.h"
#include "ColorEnhancer.h"
#include "IRegionOfInterest.h"
#include "Tiles.h"
#include "TilesCleaner.h"
#include "MatchSolver.h"
#include "MosaicBuilder.h"
#include <memory>


class MosaicGenerator
{
public:
    MosaicGenerator(const Parameters& parameters);
    ~MosaicGenerator();
    void Build();

private:
    std::shared_ptr<Photo> _photo;
    std::shared_ptr<ColorEnhancer> _colorEnhancer;
    std::shared_ptr<IRegionOfInterest> _roi;
    std::shared_ptr<Tiles> _tiles;
    std::shared_ptr<TilesCleaner> _tilesCleaner;
    std::shared_ptr<MatchSolver> _matchSolver;
    std::shared_ptr<MosaicBuilder> _mosaicBuilder;
};