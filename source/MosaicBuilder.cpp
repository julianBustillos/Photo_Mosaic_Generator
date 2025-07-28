#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
#include "ColorEnhancer.h"
#include "GaussianMixtureModel.h"


MosaicBuilder::MosaicBuilder(std::tuple<int, int> grid, std::tuple<double, double, double> blending) :
    _gridWidth(std::get<0>(grid)), _gridHeight(std::get<1>(grid)), _blendingStep(std::get<0>(blending)), _blendingMin(std::get<1>(blending)), _blendingMax(std::get<2>(blending))
{
}

MosaicBuilder::~MosaicBuilder()
{
}

void MosaicBuilder::build(const Photo& photo, const Tiles& tiles, const MatchSolver& matchSolver)
{
    const std::vector<int>& tileIds = matchSolver.getUniqueIds();
    const int gridSize = _gridWidth * _gridHeight;
    const double blendingSize = _blendingMax - _blendingMin;
    const int nbSteps = blendingSize > 0 ? (int)(blendingSize / _blendingStep) + 1 : 1;
    Console::Out::initBar("Building mosaics          ", tileIds.size() + 2 * gridSize + nbSteps);
    Console::Out::startBar(Console::DEFAULT);

    //Compute GMMs for all photo tiles
    std::vector<ProbaUtils::GMMNDComponents<3>> photoTileGmm(gridSize);
    #pragma omp parallel for
    for (int mosaicId = 0; mosaicId < gridSize; mosaicId++)
    {
        cv::Mat photoTile = photo.getTile(mosaicId);
        photoTile.convertTo(photoTile, CV_64FC3);

        ProbaUtils::SampleData<3> sampleData;
        ProbaUtils::computeSampleData(sampleData, photoTile.ptr<double>(), photoTile.rows * photoTile.cols);
        GaussianMixtureModel<3>::findOptimalComponents(photoTileGmm[mosaicId], sampleData, 1, MaxNbCompo, NbInit, MaxIter, ConvergenceTol, CovarianceReg, true);
        Console::Out::addBarSteps(1);
    }

    //Compute GMMs for all unique tiles
    std::map<int, TileData> tilesData;
    for (int tileId: tileIds)
    {
        tilesData.emplace(tileId, TileData());
    }

    #pragma omp parallel for
    for (int t = 0; t < tileIds.size(); t++)
    {
        auto& tileData = tilesData[tileIds[t]];
        const std::string tilePath = tiles.getTileFilepath(tileIds[t]);

        cv::Mat tile = cv::imread(tilePath);
        if (tile.empty())
            throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);
        tile.convertTo(tile, CV_64FC3);

        ProbaUtils::computeSampleData(tileData._sampleData, tile.ptr<double>(), tile.rows * tile.cols);
        GaussianMixtureModel<3>::findOptimalComponents(tileData._gmm, tileData._sampleData, MaxNbCompo, MaxNbCompo, NbInit, MaxIter, ConvergenceTol, CovarianceReg, true);
        Console::Out::addBarSteps(1);
    }

    //Compute color enhancer data for all tile / photo tile pairs and apply color transformation with blending
    const cv::Size tileSize = photo.getTileSize();
    cv::Size mosaicSize = tileSize;
    mosaicSize.width *= _gridWidth;
    mosaicSize.height *= _gridHeight;
    std::vector<cv::Mat> mosaics;
    mosaics.reserve(nbSteps);
    for (int s = 0; s < nbSteps; s++)
        mosaics.emplace_back(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));

    ColorEnhancer::initializeColorSpace(0, 255, 256, 16);

    #pragma omp parallel for
    for (int mosaicId = 0; mosaicId < gridSize; mosaicId++)
    {
        int tileId = matchSolver.getMatchingId(mosaicId);
        if (tileId < 0)
            throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);

        auto& tileData = tilesData[tileId];
        ColorEnhancer enhancer(tileData._sampleData, tileData._gmm, photoTileGmm[mosaicId]);

        for (int s = 0; s < nbSteps; s++)
        {
            double blending = _blendingMin + s * _blendingStep;
            cv::Mat enhancedTile(tileSize, CV_64FC3);
            enhancer.apply(enhancedTile, blending);
            copyTileOnMosaic(mosaics[s], enhancedTile, mosaicId, photo.getTileBox(mosaicId));
        }
        Console::Out::addBarSteps(1);
    }
    
    #pragma omp parallel for
    for (int s = 0; s < nbSteps; s++) 
    {
        double blending = _blendingMin + s * _blendingStep;
        exportMosaic(photo.getDirectory(), blending, mosaics[s]);
        Console::Out::addBarSteps(1);
    }

    Console::Out::waitBar();
    Log::Logger::get().log(Log::TRACE) << "Mosaic computed.";
}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const cv::Mat& tile, int mosaicId, const cv::Rect& box)
{
    const int channels = tile.channels();
    int pt = 0;
    const int step = 3 * (mosaic.cols - box.width);
    int pm = channels * (box.y * mosaic.cols + box.x);
    for (int i = 0; i < tile.rows; i++, pm += step)
        for (int j = 0; j < tile.cols; j++)
            for (int c = 0; c < channels; c++, pt++, pm++)
                mosaic.data[pm] = (uchar)tile.data[pt];
}

void MosaicBuilder::exportMosaic(const std::string& path, double blending, const cv::Mat mosaic)
{
    std::string value = std::to_string((int)(blending * 100));
    value = std::string(3 - value.length(), '0') + value;
    std::string mosaicPath = (path.empty() ? "" : path + "\\") + "mosaic_" + value + ".jpg";
    cv::imwrite(mosaicPath, mosaic, std::vector<int>({MosaicParam[0], MosaicParam[1]}));
    Log::Logger::get().log(Log::INFO) << "Mosaic exported at " << mosaicPath;
}

