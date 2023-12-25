#include "MosaicBuilder.h"
#include <vector>
#include "CustomException.h"
#include "Utils.h"


void MosaicBuilder::build(const IPixelAdapter& pixelAdapter, const ITiles& tiles, const IMatchSolver& matchSolver)
{
    cv::Size mosaicSize = _photo->getTileSize() * _subdivisions;
    cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
    uchar* mosaicData = mosaic.data;
    const std::vector<int>& matchingTiles = matchSolver.getMatchingTiles();

    for (int i = 0; i < _subdivisions; i++)
    {
        for (int j = 0; j < _subdivisions; j++)
        {
            int mosaicId = i * _subdivisions + j;
            int tileId = matchingTiles[mosaicId];
            if (tileId >= 0)
                copyTileOnMosaic(mosaicData, tiles.getTileFilepath(tileId), pixelAdapter, mosaicId, _photo->getFirstPixel(i, j, false), mosaicSize.width);
            else
                throw CustomException("One or several tiles missing from match solver !", CustomException::Level::NORMAL);
        }
    }

    exportMosaic(_photo->getDirectory(), mosaic);

    printInfo();
}

void MosaicBuilder::copyTileOnMosaic(uchar* mosaicData, const std::string& tilePath, const IPixelAdapter& pixelAdapter, int mosaicId, const cv::Point firstPixel, int step)
{
    cv::Mat tile = cv::imread(tilePath);
    uchar* tileData = tile.data;
    if (!tileData)
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::NORMAL);

    pixelAdapter.applyCorrection(tile, mosaicId);

    cv::Size tileSize = tile.size();
    for (int i = 0; i < tileSize.height; i++)
    {
        for (int j = 0; j < tileSize.width; j++)
        {
            int mosaicId = Utils::getDataIndex(firstPixel.y + i, firstPixel.x + j, step);
            int tileId = Utils::getDataIndex(i, j, tileSize.width);
            for (int c = 0; c < 3; c++)
            {
                mosaicData[mosaicId + c] = tileData[tileId + c];
            }
        }
    }
}

void MosaicBuilder::exportMosaic(const std::string& path, const cv::Mat mosaic)
{
    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    image_params.push_back(100);
    cv::imwrite(path + "\\mosaic.jpg", mosaic, image_params);
}

void MosaicBuilder::printInfo() const
{
    std::cout << "MOSAIC BUILDER :" << std::endl;
    std::cout << "TODO !!" << std::endl;
    std::cout << std::endl;
}
