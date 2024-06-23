#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include "OutputManager.h"
#include "ProgressBar.h"
#include "Log.h"
#include <stack>
#include <thread>//TODO DEBUG


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    const unsigned int maxBitDist = MathUtils::HashBits * DistanceTolerance;
    cv::Mat tile;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());
    std::vector<bool> isEmpty(tiles.getNbTiles(), false);
    std::vector<bool> isDuplicate(tiles.getNbTiles(), false);

    ProgressBar progressBar("Detect image duplicates ", 60);//TODO DEBUG
    progressBar.setNbSteps(tiles.getNbTiles() + 2);
    std::thread barThread(&ProgressBar::threadExecution, &progressBar);//TODO DEBUG

    OutputManager::getInstance().cstderr_silent();
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, tile);
        if (!tile.empty())
        {
            MathUtils::computeImageDHash(tile, hashes[t]);
        }   
        else
        {
            isEmpty[t] = true;
        }
        progressBar.addSteps(1);
    }
    OutputManager::getInstance().cstderr_restore();
    Log::Logger::getInstance().log(Log::TRACE) << "Tiles DHash computed.";

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
    progressBar.addSteps(1);
    Log::Logger::getInstance().log(Log::TRACE) << "Tiles DHash compared.";

    std::vector<unsigned int> toRemove;
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        if (isEmpty[t] || isDuplicate[t])
            toRemove.emplace_back(t);
    }
    tiles.remove(toRemove);
    progressBar.addSteps(1);
    Log::Logger::getInstance().log(Log::TRACE) << toRemove.size() << " tiles removed.";
    Log::Logger::getInstance().log(Log::TRACE) << tiles.getNbTiles() << " remaining tiles.";

    barThread.join();
}

