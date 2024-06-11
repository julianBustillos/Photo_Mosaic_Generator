#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include "OutputManager.h"
#include <stack>


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    const unsigned int maxBitDist = MathUtils::HashBits * DistanceTolerance;
    cv::Mat tile;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());
    std::vector<bool> isEmpty(tiles.getNbTiles(), false);
    std::vector<bool> isDuplicate(tiles.getNbTiles(), false);

    OutputManager::getInstance().cstderr_silent();
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, tile);
        if (tile.empty())
        {
            isEmpty[t] = true;
            continue;
        }   
        MathUtils::computeImageDHash(tile, hashes[t]);
    }
    OutputManager::getInstance().cstderr_restore();

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

    std::vector<unsigned int> toRemove;
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        if (isEmpty[t] || isDuplicate[t])
            toRemove.emplace_back(t);
    }
    tiles.remove(toRemove);
}

