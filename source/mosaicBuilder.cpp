#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "MathUtils.h"


void MosaicBuilder::build(const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver)
{
    cv::Size mosaicSize = _photo->getTileSize() * _subdivisions;
    cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
    const std::vector<int>& matchingTiles = matchSolver.getMatchingTiles();

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            int tileId = matchingTiles[mosaicId];
            if (tileId >= 0)
                copyTileOnMosaic(mosaic, tiles.getTileFilepath(tileId), pixelAdapter, mosaicId, _photo->getTileBox(i, j, false));
            else
                throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);
        }
    }

    exportMosaic(_photo->getDirectory(), mosaic);

    printInfo();
}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Rect& box)
{
    cv::Mat tile = cv::imread(tilePath);
    if (tile.empty())
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);

    pixelAdapter.applyCorrection(tile, mosaicId);

    cv::Size tileSize = tile.size();
    for (int i = 0; i < tileSize.height; i++)
    {
        for (int j = 0; j < tileSize.width; j++)
        {
            for (int c = 0; c < tile.channels(); c++)
            {
                mosaic.ptr(box.y + i, box.x + j)[c] = tile.ptr(i, j)[c];
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

void MosaicBuilder::printInfo() const
{
    std::cout << "MOSAIC BUILDER :" << std::endl;
    std::cout << "TODO !!" << std::endl;
    std::cout << std::endl;
}
