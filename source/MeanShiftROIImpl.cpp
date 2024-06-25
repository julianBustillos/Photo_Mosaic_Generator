#include "MeanShiftROIImpl.h"
#include "MeanShift.h"
#include "SaliencyFilter.h"




MeanSHiftROIImpl::MeanSHiftROIImpl()
{
}

void MeanSHiftROIImpl::find(const cv::Mat& image, cv::Rect& box, bool rowDirSearch, int threadID) const
{
    std::vector<int> clusterMapping;
    int nbClusters;
    std::vector<double> saliency;
    int iMean, jMean;
    bool saliencyFound;

    MeanShift::compute(image, clusterMapping, nbClusters);
    SaliencyFilter::compute(image, clusterMapping, nbClusters, saliency, iMean, jMean, saliencyFound);

    if (saliencyFound)
    {
        if (rowDirSearch)
            findRowROI(image.size(), clusterMapping, saliency, jMean, box);
        else
            findColROI(image.size(), clusterMapping, saliency, iMean, box);
    }
    else
    {
        getDefaultROI(image.size(), box, rowDirSearch);
    }
}

void MeanSHiftROIImpl::getDefaultROI(const cv::Size& imageSize, cv::Rect& box, bool rowDirSearch) const
{
    box.x = (int)(floor((imageSize.width - box.width) / 2.));
    if (rowDirSearch)
        box.y = (int)(floor((imageSize.height - box.height) / 2.));
    else
        box.y = (int)(floor((imageSize.height - box.height) / 3.));
}

void MeanSHiftROIImpl::findRowROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int jMean, cv::Rect& box) const
{
    double saliencyAccumulator = 0.;
    double maxRegionSaliency;
    const int meanRegionFirstPixel = jMean - (int)(box.width / 2.);
    int regionCenterDistToMean;
    box.y = 0;

    //Initialize accumulator
    for (int i = 0; i < imageSize.height; i++)
    {
        for (int j = 0; j < box.width; j++)
        {
            int currentId = i * imageSize.width + j;
            saliencyAccumulator += saliency[clusterMapping[currentId]];
        }
    }
    maxRegionSaliency = saliencyAccumulator;
    regionCenterDistToMean = meanRegionFirstPixel;
    box.x = 0;

    //Find best Region according to cropSize
    for (int j = 1; j < imageSize.width - box.width; j++)
    {
        for (int i = 0; i < imageSize.height; i++)
        {
            int subId = i * imageSize.width + j - 1;
            int addId = i * imageSize.width + j + box.width;

            saliencyAccumulator -= saliency[clusterMapping[subId]];
            saliencyAccumulator += saliency[clusterMapping[addId]];
        }

        if (saliencyAccumulator > maxRegionSaliency)
        {
            maxRegionSaliency = saliencyAccumulator;
            regionCenterDistToMean = meanRegionFirstPixel - j;
            box.x = j;
        }
        else if (saliencyAccumulator == maxRegionSaliency)
        {
            if (abs(meanRegionFirstPixel - j) < regionCenterDistToMean)
            {
                maxRegionSaliency = saliencyAccumulator;
                regionCenterDistToMean = meanRegionFirstPixel - j;
                box.x = j;
            }
        }
    }
}

void MeanSHiftROIImpl::findColROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int iMean, cv::Rect& box) const
{
    double saliencyAccumulator = 0.;
    double maxRegionSaliency;
    const int meanRegionFirstPixel = iMean - (int)(box.height / 2.);
    int regionCenterDistToMean;
    box.x = 0;

    //Initialize accumulator
    for (int i = 0; i < box.height; i++)
    {
        for (int j = 0; j < imageSize.width; j++)
        {
            int currentId = i * imageSize.width + j;
            saliencyAccumulator += saliency[clusterMapping[currentId]];
        }
    }
    maxRegionSaliency = saliencyAccumulator;
    regionCenterDistToMean = meanRegionFirstPixel;
    box.y = 0;

    //Find best Region according to cropSize
    for (int i = 1; i < imageSize.height - box.height; i++)
    {
        for (int j = 0; j < imageSize.width; j++)
        {
            int subId = (i - 1) * imageSize.width + j;
            int addId = (i + box.height) * imageSize.width + j;

            saliencyAccumulator -= saliency[clusterMapping[subId]];
            saliencyAccumulator += saliency[clusterMapping[addId]];
        }

        if (saliencyAccumulator > maxRegionSaliency)
        {
            maxRegionSaliency = saliencyAccumulator;
            regionCenterDistToMean = meanRegionFirstPixel - i;
            box.y = i;
        }
        else if (saliencyAccumulator == maxRegionSaliency)
        {
            if (abs(meanRegionFirstPixel - i) < regionCenterDistToMean)
            {
                maxRegionSaliency = saliencyAccumulator;
                regionCenterDistToMean = meanRegionFirstPixel - i;
                box.y = i;
            }
        }
    }
}

