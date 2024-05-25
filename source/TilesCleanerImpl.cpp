#include "TilesCleanerImpl.h"
#include "MathUtils.h"
#include <stack>


void TilesCleanerImpl::clean(ITiles& tiles) const
{
    cleanBlurries(tiles);
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
    MathUtils::Hash hash;
    std::vector<MathUtils::Hash> hashes(tiles.getNbTiles());

    for (int t = 0; t < tiles.getNbTiles(); t++)
    {
        tiles.getImage(t, image);
        MathUtils::computeImageDHash(image, hashes[t]);
    }

    std::vector<std::vector<unsigned int>> similarityGraph(tiles.getNbTiles());
    for (int t1 = 0; t1 < tiles.getNbTiles() - 1; t1++)
    {
        for (int t2 = t1 + 1; t2 < tiles.getNbTiles(); t2++)
        {
            hash = hashes[t1];
            hash ^= hashes[t2];
            if (hash.count() <= maxBitDist)
            {
                similarityGraph[t1].emplace_back(t2);
                similarityGraph[t2].emplace_back(t1);
            }
        }
    }

    int currGroup = -1;
    std::vector<int> duplicateGroup(tiles.getNbTiles(), -1);
    std::stack<unsigned int> groupStack;
    for (int i = 0; i < similarityGraph.size(); i++)
    {
        if (!similarityGraph[i].empty())
        {
            currGroup++;
            groupStack.emplace(i);

            while (!groupStack.empty())
            {
                int j = groupStack.top();
                groupStack.pop();
                if (!similarityGraph[j].empty())
                {
                    duplicateGroup[j] = currGroup;
                    for (int k = 0; k < similarityGraph[j].size(); k++)
                    {
                        groupStack.emplace(similarityGraph[j][k]);
                    }
                    similarityGraph[j].clear();
                }
            }
        }
    }

    std::vector<unsigned int> toRemove;
    std::vector<bool> isNewGroup(currGroup, true);
    for (int i = 0; i < duplicateGroup.size(); i++)
    {
        if (duplicateGroup[i] >= 0)
        {
            if (isNewGroup[duplicateGroup[i]])
            {
                isNewGroup[duplicateGroup[i]] = false;

            }
            else
            {
                toRemove.emplace_back(i);
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
