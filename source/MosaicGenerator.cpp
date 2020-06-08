#include "MosaicGenerator.h"
#include "PixelAdapterImpl.h"
#include "TilesImpl.h"
#include "MatchSolverImpl.h"
#include "CustomException.h"


MosaicGenerator::MosaicGenerator(const Parameters & parameters) :
    _photo(parameters.getPhotoPath(), parameters.getWidth(), parameters.getHeight(), parameters.getSubdivision()),
    _mosaicBuilder(_photo, parameters.getSubdivision())
{
    _pixelAdapter = new PixelAdapterImpl(_photo, parameters.getSubdivision());
    if (!_pixelAdapter)
        throw CustomException("Bad allocation for _pixelAdapter in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _tiles = new TilesImpl(parameters.getTilesPath(), _photo.getTileSize());
    if (!_tiles)
        throw CustomException("Bad allocation for _tiles in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _matchSolver = new MatchSolverImpl(_photo, parameters.getSubdivision());
    if (!_matchSolver)
        throw CustomException("Bad allocation for _matchSolver in MosaicGenerator constructor.", CustomException::Level::ERROR);
}

MosaicGenerator::~MosaicGenerator()
{
    if (_pixelAdapter)
        delete _pixelAdapter;
    _pixelAdapter = nullptr;
    if (_tiles)
        delete _tiles;
    _tiles = nullptr;
    if (_matchSolver)
        delete _matchSolver;
    _matchSolver = nullptr;
}

void MosaicGenerator::Build()
{
    _pixelAdapter->compute();
    _matchSolver->solve(*_tiles);
    _mosaicBuilder.Build(*_pixelAdapter, *_tiles, *_matchSolver);
}
