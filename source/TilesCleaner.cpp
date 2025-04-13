#include "TilesCleaner.h"
#include "ImageUtils.h"
#include "OutputManager.h"
#include "ProgressBar.h"
#include "Log.h"
#include "Console.h"
#include <stack>


void TilesCleaner::clean(Tiles& tiles) const
{
    const unsigned int maxBitDist = (unsigned int)(ImageUtils::HashBits * DistanceTolerance);
    std::vector<ImageUtils::Hash> hashes(tiles.getNbTiles());
    std::vector<bool> isEmpty(tiles.getNbTiles(), false);
    std::vector<bool> isDuplicate(tiles.getNbTiles(), false);

    Console::Out::initBar("Detecting image duplicates", tiles.getNbTiles() + 2);
    Console::Out::startBar(Console::DEFAULT);

    OutputManager::get().cstderr_silent();
    #pragma omp parallel for
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        cv::Mat tile;
        tiles.getImage(t, tile);
        if (!tile.empty())
        {
            ImageUtils::DHash(tile, hashes[t]);
        }   
        else
        {
            isEmpty[t] = true;
        }
        Console::Out::addBarSteps(1);
    }
    OutputManager::get().cstderr_restore();
    Log::Logger::get().log(Log::TRACE) << "Tiles DHash computed.";

    for (int t1 = 0; t1 < tiles.getNbTiles() - 1; t1++)
    {
        if (isEmpty[t1])
            continue;
        for (int t2 = t1 + 1; t2 < tiles.getNbTiles(); t2++)
        {
            if (isEmpty[t2])
                continue;
            if ((hashes[t1] ^ hashes[t2]).count() <= maxBitDist)
            {
                isDuplicate[t2] = true;
            }
        }
    }
    Console::Out::addBarSteps(1);
    Log::Logger::get().log(Log::TRACE) << "Tiles DHash compared.";

    std::vector<unsigned int> toRemove;
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        if (isEmpty[t] || isDuplicate[t])
            toRemove.emplace_back(t);
    }
    tiles.remove(toRemove);
    Console::Out::addBarSteps(1);
    Log::Logger::get().log(Log::TRACE) << toRemove.size() << " tiles removed.";
    Log::Logger::get().log(Log::TRACE) << tiles.getNbTiles() << " remaining tiles.";

    Console::Out::waitBar();
}

