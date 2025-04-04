#include "MosaicGenerator.h"
#include "PixelAdapter.h"
#include "FaceDetectionROIImpl.h"
#include "CustomException.h"
#include "Console.h"


MosaicGenerator::MosaicGenerator(const Parameters& parameters)
{
    _photo = std::make_shared<Photo>(parameters.getPhotoPath(), parameters.getGrid(), parameters.getScale(), parameters.getResolution());
    if (!_photo)
        throw CustomException("Bad allocation for _photo in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _pixelAdapter = std::make_shared<PixelAdapter>(parameters.getGrid());
    if (!_pixelAdapter)
        throw CustomException("Bad allocation for _pixelAdapter in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _roi = std::make_shared<FaceDetectionROIImpl>();
    if (!_roi)
        throw CustomException("Bad allocation for _roi in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _tiles = std::make_shared<Tiles>(parameters.getTilesPath(), parameters.getGrid());
    if (!_tiles)
        throw CustomException("Bad allocation for _tiles in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _tilesCleaner = std::make_shared<TilesCleaner>();
    if (!_tilesCleaner)
        throw CustomException("Bad allocation for _tilesCleaner in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _matchSolver = std::make_shared<MatchSolver>(parameters.getGrid());
    if (!_matchSolver)
        throw CustomException("Bad allocation for _matchSolver in MosaicGenerator constructor.", CustomException::Level::ERROR);

    _mosaicBuilder = std::make_shared<MosaicBuilder>(parameters.getGrid(), parameters.getBlending());
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
    Console::Out::get(Console::DEFAULT) << "Initializing data...";
    _photo->initialize();
    _roi->initialize();
    _tiles->initialize(_matchSolver->getRequiredNbTiles());

    _tilesCleaner->clean(*_tiles);
    _tiles->compute(*_roi, *_photo);
    _pixelAdapter->compute(*_photo);
    _matchSolver->solve(*_tiles);
    _mosaicBuilder->build(*_photo, *_pixelAdapter, *_tiles, *_matchSolver);
}
