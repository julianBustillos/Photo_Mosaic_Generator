#pragma once

#include "Parameters.h"
#include "Photo.h"
#include "ColorEnhancer.h"
#include "FaceDetectionROI.h"
#include "Tiles.h"
#include "DuplicateRemover.h"
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
    std::shared_ptr<FaceDetectionROI> _roi;
    std::shared_ptr<Tiles> _tiles;
    std::shared_ptr<DuplicateRemover> _duplicateRemover;
    std::shared_ptr<MatchSolver> _matchSolver;
    std::shared_ptr<MosaicBuilder> _mosaicBuilder;
};