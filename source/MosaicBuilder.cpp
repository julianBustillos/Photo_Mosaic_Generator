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

//DEBUG
int dump_i, dump_j;
const Photo* dump_photo = NULL;
//DEBUG
void MosaicBuilder::build(const Photo& photo, const ColorEnhancer& colorEnhancer, const Tiles& tiles, const MatchSolver& matchSolver)
{
    Console::Out::get(Console::DEFAULT) << "Building mosaics...";
    cv::Size mosaicSize = photo.getTileSize();
    mosaicSize.width *= _gridWidth;
    mosaicSize.height *= _gridHeight;
    const std::vector<int>& matchingTiles = matchSolver.getMatchingTiles();
    const double blendingSize = _blendingMax - _blendingMin;
    const int nbSteps = blendingSize > 0 ? (int)(blendingSize / _blendingStep) + 1 : 1;

    //#pragma omp parallel for
    for (int s = 0; s < nbSteps; s++)
    {
        cv::Mat mosaic(mosaicSize, CV_8UC3, cv::Scalar(0, 0, 0));
        double blendingValue = _blendingMin + s * _blendingStep;
        for (int i = 0; i < _gridHeight; i++)
        {
            for (int j = 0; j < _gridWidth; j++)
            {
                int mosaicId = i * _gridWidth + j;
                int tileId = matchingTiles[mosaicId];

                //DEBUG
                dump_i = i;
                dump_j = j;
                dump_photo = &photo;
                //DEBUG

                if (tileId >= 0)
                    copyTileOnMosaic(mosaic, tiles.getTileFilepath(tileId), colorEnhancer, blendingValue, mosaicId, photo.getTileBox(i, j, false));
                else
                    throw CustomException("One or several tiles missing from match solver !", CustomException::Level::ERROR);
            }
        }
        exportMosaic(photo.getDirectory(), blendingValue, mosaic);
    }

    Log::Logger::get().log(Log::TRACE) << "Mosaic computed.";

}

void MosaicBuilder::copyTileOnMosaic(cv::Mat& mosaic, const std::string& tilePath, const ColorEnhancer& colorEnhancer, double blending, int mosaicId, const cv::Rect& box)
{
    cv::Mat tile = cv::imread(tilePath);
    if (tile.empty())
        throw CustomException("Impossible to find temporary exported tile : " + tilePath, CustomException::Level::ERROR);

    //DEBUG
    if (blending == 0)
    {
        std::string dumpFolder = "C:\\Users\\JulianBustillos\\Downloads\\MOSAIC_ANAELLE\\DUMP_COLORS\\";
        std::string value_i = std::to_string((int)(dump_i));
        value_i = std::string(3 - value_i.length(), '0') + value_i;
        std::string value_j = std::to_string((int)(dump_j));
        value_j = std::string(3 - value_j.length(), '0') + value_j;

        std::string path = dumpFolder + "tile_" + value_i + "_" + value_j + ".jpg";
        //cv::imwrite(path, tile, std::vector<int>({MosaicParam[0], MosaicParam[1]}));

        cv::Mat reference = tile.clone();
        cv::Rect box = dump_photo->getTileBox(dump_i, dump_j, true);
        const cv::Size refSize = reference.size();

        for (int i = 0; i < box.height; i++)
        {
            for (int j = 0; j < box.width; j++)
            {
                int refID = (i * box.width + j) * 3;
                int photoID = ((i + box.y) * dump_photo->getImage().size().width + (j + box.x)) * 3;
                for (int c = 0; c < 3; c++)
                {
                    reference.data[refID + c] = dump_photo->getImage().data[photoID + c];
                }
            }
        }

        path = dumpFolder + "reference_" + value_i + "_" + value_j + ".jpg";
        //cv::imwrite(path, reference, std::vector<int>({MosaicParam[0], MosaicParam[1]}));

        if (dump_i == 25 && dump_j == 33)
        {
            for (int c = 0; c < 3; c++)
            {
                std::vector<int> data;
                data.reserve(box.width * box.height);
                for (int d = 0; d < box.width * box.height; d++)
                {
                    data.push_back(reference.data[3 * d + c]);
                }

                std::vector<GaussianMixtureModel::Component> components = GaussianMixtureModel::findOptimalComponents(data, 10, 1e-3, 300, 1e-3, 1000, true);
            }


        }
    }
    //DEBUG

    colorEnhancer.apply(tile, blending, mosaicId);

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
    std::string value = std::to_string((int)(blending * 100));
    value = std::string(3 - value.length(), '0') + value;
    std::string mosaicPath = (path.empty() ? "" : path + "\\") + "mosaic_" + value + ".jpg";
    cv::imwrite(mosaicPath, mosaic, std::vector<int>({MosaicParam[0], MosaicParam[1]}));
    Log::Logger::get().log(Log::INFO) << "Mosaic exported at " << mosaicPath;
}

