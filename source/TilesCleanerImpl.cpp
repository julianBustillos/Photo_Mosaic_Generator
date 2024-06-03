#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include <stack>


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    //cleanBlurries(tiles); //TODO MERGE WITH dup for PERFORMANCES
    cleanDuplicates(tiles);
}

void TilesCleanerImpl::cleanBlurries(ITiles& tiles) const
{
    cv::Mat image;
    std::vector<unsigned int> toRemove;

    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, image);
        if (isBlurry(image))
        {
            toRemove.emplace_back(t);
        }
    }
    tiles.remove(toRemove);
}

void TilesCleanerImpl::cleanDuplicates(ITiles& tiles) const
{
    const unsigned int maxBitDist = MathUtils::HashBits * DistanceTolerance;
    cv::Mat image;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());

    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, image);
        MathUtils::computeImageDHash(image, hashes[t]);
    }

    std::vector<unsigned int> toRemove;
    for (int t1 = 0; t1 < tiles.getNbTiles() - 1; t1++)
    {
        for (int t2 = t1 + 1; t2 < tiles.getNbTiles(); t2++)
        {
            if ((hashes[t1] ^ hashes[t2]).count() <= maxBitDist)
            {
                toRemove.emplace_back(t2);
            }
        }
    }

    tiles.remove(toRemove);
}

bool TilesCleanerImpl::isBlurry(const cv::Mat& image) const
{
    //TODO
    return false;
}
