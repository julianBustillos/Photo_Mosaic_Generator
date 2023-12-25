#include "MosaicGenerator.h"
#include "PixelAdapterImpl.h"
#include "FaceDetectionROIImpl.h"
#include "TilesImpl.h"
#include "MatchSolverImpl.h"
#include "CustomException.h"


MosaicGenerator::MosaicGenerator(const Parameters& parameters)
{
    _photo = std::make_shared<const Photo>(parameters.getPhotoPath(), parameters.getScale(), parameters.getRatio(), parameters.getSubdivision());
    if (!_matchSolver)
        throw CustomException("Bad allocation for _matchSolver in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _pixelAdapter = std::make_shared<PixelAdapterImpl>(_photo, parameters.getSubdivision());
    if (!_pixelAdapter)
        throw CustomException("Bad allocation for _pixelAdapter in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _roi = std::make_shared<FaceDetectionROIImpl>();
    if (!_roi)
        throw CustomException("Bad allocation for _roi in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _tiles = std::make_shared<TilesImpl>(parameters.getTilesPath(), _photo->getTileSize());
    if (!_tiles)
        throw CustomException("Bad allocation for _tiles in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _matchSolver = std::make_shared<MatchSolverImpl>(_photo, parameters.getSubdivision());
    if (!_matchSolver)
        throw CustomException("Bad allocation for _matchSolver in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _mosaicBuilder = std::make_shared<MosaicBuilder>(_photo, parameters.getSubdivision());
    if (!_mosaicBuilder)
        throw CustomException("Bad allocation for _mosaicBuilder in MosaicGenerator constructor.", CustomException::Level::ERROR);
}

MosaicGenerator::~MosaicGenerator()
{
    _photo.reset();
    _pixelAdapter.reset();
    _roi.reset();
    _tiles.reset();
    _matchSolver.reset();
    _mosaicBuilder.reset();
}

void MosaicGenerator::Build()
{
    _tiles->compute(*_roi);
    _pixelAdapter->compute();
    _matchSolver->solve(*_tiles);
    _mosaicBuilder->build(*_pixelAdapter, *_tiles, *_matchSolver);
}
