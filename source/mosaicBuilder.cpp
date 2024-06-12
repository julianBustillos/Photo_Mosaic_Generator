#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "MathUtils.h"


MosaicBuilder::MosaicBuilder(int subdivisions) : 
    _subdivisions(subdivisions)
{
}

MosaicBuilder::~MosaicBuilder()
{
}

void MosaicBuilder::build(const Photo& photo, const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver)
{
    cv::Size mosaicSize = photo.getTileSize() * _subdivisions;
    cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
    const std::vector<int>& matchingTiles = matchSolver.getMatchingTiles();

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            int tileId = matchingTiles[mosaicId];
            if (tileId >= 0)
                copyTileOnMosaic(mosaic, tiles.getTileFilepath(tileId), pixelAdapter, mosaicId, photo.getTileBox(i, j, false));
            else
                throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);
        }
    }

    exportMosaic(photo.getDirectory(), mosaic);
}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Rect& box)
{
    cv::Mat tile = cv::imread(tilePath);
    if (tile.empty())
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);

    pixelAdapter.applyCorrection(tile, mosaicId);

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

void MosaicBuilder::exportMosaic(const std::string& path, const cv::Mat mosaic)
{
    std::vector<int> image_params;
    image_params.emplace_back(cv::IMWRITE_JPEG_QUALITY);
    image_params.emplace_back(100);
    cv::imwrite(path + "\\mosaic.jpg", mosaic, image_params);
}

