#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "MathUtils.h"
#include "Log.h"
#include "Console.h"


MosaicBuilder::MosaicBuilder(int subdivisions, double blending, double blendingMin, double blendingMax) :
    _subdivisions(subdivisions), _blending(blending), _blendingMin(blendingMin), _blendingMax(blendingMax)
{
}

MosaicBuilder::~MosaicBuilder()
{
}

void MosaicBuilder::build(const Photo& photo, const PixelAdapter& pixelAdapter, const Tiles& tiles, const MatchSolver& matchSolver)
{
    Console::Out::get(Console::DEFAULT) << "Building mosaics...";
    cv::Size mosaicSize = photo.getTileSize() * _subdivisions;
    const std::vector<int>& matchingTiles = matchSolver.getMatchingTiles();
    const double blendingSize = _blendingMax - _blendingMin;
    const int nbSteps = blendingSize > 0 ? (int)(blendingSize / _blending) + 1 : 1;

    #pragma omp parallel for
    for (int b = 0; b < nbSteps; b++)
    {
        cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
        double blendingValue = _blendingMin + b * _blending;
        for (int i = 0; i < _subdivisions; i++)
        {
            for (int j = 0; j < _subdivisions; j++)
            {
                int mosaicId = i * _subdivisions + j;
                int tileId = matchingTiles[mosaicId];
                if (tileId >= 0)
                    copyTileOnMosaic(mosaic, tiles.getTileFilepath(tileId), pixelAdapter, blendingValue, mosaicId, photo.getTileBox(i, j, false));
                else
                    throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);
            }
        }
        exportMosaic(photo.getDirectory(), blendingValue, mosaic);
    }

    Log::Logger::get().log(Log::TRACE) << "Mosaic computed.";

}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const PixelAdapter& pixelAdapter, double blending, int mosaicId, const cv::Rect& box)
{
    cv::Mat tile = cv::imread(tilePath);
    if (tile.empty())
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);

    pixelAdapter.applyCorrection(tile, blending, mosaicId);

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
                mosaic.data[pm] = tile.data[pt];
            }
        }
    }
}

void MosaicBuilder::exportMosaic(const std::string& path, double blending, const cv::Mat mosaic)
{
    std::vector<int> image_params;
    image_params.emplace_back(cv::IMWRITE_JPEG_QUALITY);
    image_params.emplace_back(100);
    std::string value = std::to_string((int)(blending * 100));
    value = std::string(3 - value.length(), '0') + value;
    std::string mosaicPath = (path.empty() ? "" : path + "\\") + "mosaic_" + value + ".jpg";
    cv::imwrite(mosaicPath, mosaic, image_params);
    Log::Logger::get().log(Log::INFO) << "Mosaic exported at " << mosaicPath;
}

