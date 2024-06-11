#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include "OutputManager.h"
#include <stack>


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    const unsigned int maxBitDist = MathUtils::HashBits * DistanceTolerance;
    cv::Mat tile, grayscale;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());
    std::vector<bool> firstPass(tiles.getNbTiles(), false); // Empty and blurry images
    std::vector<bool> secondPass(tiles.getNbTiles(), false); // duplicate images

    OutputManager::getInstance().cstderr_silent();
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, tile);
        if (tile.empty())
        {
            firstPass[t] = true;
            continue;
        }
        MathUtils::computeGrayscale(grayscale, tile);
        if (isBlurry(grayscale))
        {
            firstPass[t] = true;
        }
        else
        {
            MathUtils::computeImageDHash(grayscale, hashes[t]);
        }
    }
    OutputManager::getInstance().cstderr_restore();

    for (int t1 = 0; t1 < tiles.getNbTiles() - 1; t1++)
    {
        if (firstPass[t1])
            continue;
        for (int t2 = t1 + 1; t2 < tiles.getNbTiles(); t2++)
        {
            if (firstPass[t2])
                continue;
            if ((hashes[t1] ^ hashes[t2]).count() <= maxBitDist)
            {
                secondPass[t2] = true;
            }
        }
    }

    std::vector<unsigned int> toRemove;
    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        if (firstPass[t] || secondPass[t])
            toRemove.emplace_back(t);
    }
    tiles.remove(toRemove);
}

bool TilesCleanerImpl::isBlurry(const cv::Mat& image) const
{
    return MathUtils::computeVarianceOfLaplacian(image) < BlurinessThreashold;
}
