#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "Log.h"
#include "Console.h"
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
    Console::Out::get(Console::DEFAULT) << "Building mosaics...";
    cv::Size mosaicSize = photo.getTileSize();
    mosaicSize.width *= _gridWidth;
    mosaicSize.height *= _gridHeight;
    const double blendingSize = _blendingMax - _blendingMin;
    const int nbSteps = blendingSize > 0 ? (int)(blendingSize / _blendingStep) + 1 : 1;
    std::vector<cv::Mat> mosaics(nbSteps, cv::Mat(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0)));

    #pragma omp parallel for
    for (int mosaicId = 0; mosaicId < _gridWidth * _gridHeight; mosaicId++)
    {
        int tileId = matchSolver.getMatchingTile(mosaicId);
        if (tileId < 0)
            throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);
        
        const std::string tilePath = tiles.getTileFilepath(tileId);
        cv::Mat tile = cv::imread(tilePath);
        if (tile.empty())
            throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);

        ColorEnhancer enhancer;
        enhancer.computeData(photo, tile, mosaicId);

        for (int s = 0; s < nbSteps; s++)
        {
            double blending = _blendingMin + s * _blendingStep;
            copyTileOnMosaic(mosaics[s], tile, enhancer, blending, mosaicId, photo.getTileBox(mosaicId, false));
        }

    }
    
    #pragma omp parallel for
    for (int s = 0; s < nbSteps; s++) 
    {
        double blending = _blendingMin + s * _blendingStep;
        exportMosaic(photo.getDirectory(), blending, mosaics[s]);
    }

    Log::Logger::get().log(Log::TRACE) << "Mosaic computed.";
}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const cv::Mat& tile, const ColorEnhancer& colorEnhancer, double blending, int mosaicId, const cv::Rect& box)
{
    const cv::Size tileSize = tile.size();
    const int channels = tile.channels();
    int pt = 0;
    const int step = 3 * (mosaic.size().width - box.width);
    int pm = channels * (box.y * mosaic.size().width + box.x);
    for (int i = 0; i < tileSize.height; i++, pm += step)
    {
        for (int j = 0; j < tileSize.width; j++)
        {
            for (int c = 0; c < channels; c++, pt++, pm++)
            {
                mosaic.data[pm] = colorEnhancer.apply(tile.data[pt], c, blending);
            }
        }
    }
}

void MosaicBuilder::exportMosaic(const std::string& path, double blending, const cv::Mat mosaic)
{
    std::string value = std::to_string((int)(blending * 100));
    value = std::string(3 - value.length(), '0') + value;
    std::string mosaicPath = (path.empty() ? "" : path + "\\") + "mosaic_" + value + ".jpg";
    cv::imwrite(mosaicPath, mosaic, std::vector<int>({MosaicParam[0], MosaicParam[1]}));
    Log::Logger::get().log(Log::INFO) << "Mosaic exported at " << mosaicPath;
}

