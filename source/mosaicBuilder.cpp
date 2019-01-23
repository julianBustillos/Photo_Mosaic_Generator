#include "mosaicBuilder.h"
#include <vector>
#include "customException.h"


MosaicBuilder::MosaicBuilder(const Photo & photo, const Tiles & tiles, int subdivisions, int * matchingTiles)
{
    cv::Size mosaicSize = photo.getTileSize() * subdivisions;
    cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
    uchar *mosaicData = mosaic.data;
    const std::vector<Tiles::Data> &tilesData = tiles.getTileDataVector();
    for (int i = 0; i < subdivisions; i++) {
        for (int j = 0; j < subdivisions; j++) {
            int tileIndex = matchingTiles[i * subdivisions + j];
            if (tileIndex >= 0)
                copyTileOnMosaic(mosaicData, tiles.getTempTilePath() + tilesData[tileIndex].filename, photo.getFirstPixel(i, j, false), mosaicSize.width);
        }
    }

    exportMosaic(photo.getDirectory(), mosaic);

    printInfo();
}

void MosaicBuilder::copyTileOnMosaic(uchar *mosaicData, const std::string & tilePath, const cv::Point firstPixelPos, const int step)
{
    cv::Mat tile = cv::imread(tilePath);
    uchar *tileData = tile.data;
    if (!tileData)
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::NORMAL);

    cv::Size tileSize = tile.size();
    for (int i = 0; i < tileSize.height; i++) {
        for (int j = 0; j < tileSize.width; j++) {
            int mosaicId = 3 * ((firstPixelPos.y + i) * step + firstPixelPos.x + j);
            int tileId = 3 * (i * tileSize.width + j);
            for (int c = 0; c < 3; c++) {
                mosaicData[mosaicId + c] = tileData[tileId + c];
            }
        }
    }
}

void MosaicBuilder::exportMosaic(const std::string &path, const cv::Mat mosaic)
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
