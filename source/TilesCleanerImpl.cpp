#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include <stack>


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    const unsigned int maxBitDist = MathUtils::HashBits * DistanceTolerance;
    cv::Mat tile, grayscale;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());

    std::vector<bool> blurry(tiles.getNbTiles(), false);
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, tile);
        if (tile.empty())
        {
            blurry[t] = true;//TODO rename this is not a blurry image removal !
            continue;
        }
        MathUtils::computeGrayscale(grayscale, tile);
        if (isBlurry(grayscale))
        {
            blurry[t] = true;
        }
        else
        {
            MathUtils::computeImageDHash(grayscale, hashes[t]);
        }
    }

    std::vector<bool> duplicate(tiles.getNbTiles(), false);
    for (int t1 = 0; t1 < tiles.getNbTiles() - 1; t1++)
    {
        if (blurry[t1])
            continue;
        for (int t2 = t1 + 1; t2 < tiles.getNbTiles(); t2++)
        {
            if (blurry[t2])
                continue;
            if ((hashes[t1] ^ hashes[t2]).count() <= maxBitDist)
            {
                duplicate[t2] = true;
            }
        }
    }

    std::vector<unsigned int> toRemove;
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        if (blurry[t] || duplicate[t])
            toRemove.emplace_back(t);
    }
    tiles.remove(toRemove);
}

bool TilesCleanerImpl::isBlurry(const cv::Mat& image) const
{
    return MathUtils::computeVarianceOfLaplacian(image) < BlurinessThreashold;
}
