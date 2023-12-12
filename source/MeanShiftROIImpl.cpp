#include "MeanShiftROIImpl.h"
#include "MeanShift.h"
#include "SaliencyFilter.h"




MeanSHiftROIImpl::MeanSHiftROIImpl()
{
}

void MeanSHiftROIImpl::find(const cv::Mat& image, cv::Point& firstPixelPos, const cv::Size& cropSize, bool rowDirSearch) const
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
            findRowROI(image.size(), clusterMapping, saliency, jMean, firstPixelPos, cropSize);
        else
            findColROI(image.size(), clusterMapping, saliency, iMean, firstPixelPos, cropSize);
    }
    else
    {
        getDefaultROI(image.size(), firstPixelPos, cropSize, rowDirSearch);
    }
}

void MeanSHiftROIImpl::getDefaultROI(const cv::Size& imageSize, cv::Point& firstPixelPos, const cv::Size& cropSize, bool rowDirSearch) const
{
    firstPixelPos.x = (int)(floor((imageSize.width - cropSize.width) / 2.));
    if (rowDirSearch)
        firstPixelPos.y = (int)(floor((imageSize.height - cropSize.height) / 2.));
    else
        firstPixelPos.y = (int)(floor((imageSize.height - cropSize.height) / 3.));
}

void MeanSHiftROIImpl::findRowROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int jMean, cv::Point& firstPixelPos, const cv::Size& cropSize) const
{
    double saliencyAccumulator = 0.;
    double maxRegionSaliency;
    const int meanRegionFirstPixel = jMean - (int)(cropSize.width / 2.);
    int regionCenterDistToMean;
    firstPixelPos.y = 0;

    //Initialize accumulator
    for (int i = 0; i < imageSize.height; i++)
    {
        for (int j = 0; j < cropSize.width; j++)
        {
            int currentId = i * imageSize.width + j;
            saliencyAccumulator += saliency[clusterMapping[currentId]];
        }
    }
    maxRegionSaliency = saliencyAccumulator;
    regionCenterDistToMean = meanRegionFirstPixel;
    firstPixelPos.x = 0;

    //Find best Region according to cropSize
    for (int j = 1; j < imageSize.width - cropSize.width; j++)
    {
        for (int i = 0; i < imageSize.height; i++)
        {
            int subId = i * imageSize.width + j - 1;
            int addId = i * imageSize.width + j + cropSize.width;

            saliencyAccumulator -= saliency[clusterMapping[subId]];
            saliencyAccumulator += saliency[clusterMapping[addId]];
        }

        if (saliencyAccumulator > maxRegionSaliency)
        {
            maxRegionSaliency = saliencyAccumulator;
            regionCenterDistToMean = meanRegionFirstPixel - j;
            firstPixelPos.x = j;
        }
        else if (saliencyAccumulator == maxRegionSaliency)
        {
            if (abs(meanRegionFirstPixel - j) < regionCenterDistToMean)
            {
                maxRegionSaliency = saliencyAccumulator;
                regionCenterDistToMean = meanRegionFirstPixel - j;
                firstPixelPos.x = j;
            }
        }
    }
}

void MeanSHiftROIImpl::findColROI(const cv::Size& imageSize, const std::vector<int>& clusterMapping, const std::vector<double>& saliency, int iMean, cv::Point& firstPixelPos, const cv::Size& cropSize) const
{
    double saliencyAccumulator = 0.;
    double maxRegionSaliency;
    const int meanRegionFirstPixel = iMean - (int)(cropSize.height / 2.);
    int regionCenterDistToMean;
    firstPixelPos.x = 0;

    //Initialize accumulator
    for (int i = 0; i < cropSize.height; i++)
    {
        for (int j = 0; j < imageSize.width; j++)
        {
            int currentId = i * imageSize.width + j;
            saliencyAccumulator += saliency[clusterMapping[currentId]];
        }
    }
    maxRegionSaliency = saliencyAccumulator;
    regionCenterDistToMean = meanRegionFirstPixel;
    firstPixelPos.y = 0;

    //Find best Region according to cropSize
    for (int i = 1; i < imageSize.height - cropSize.height; i++)
    {
        for (int j = 0; j < imageSize.width; j++)
        {
            int subId = (i - 1) * imageSize.width + j;
            int addId = (i + cropSize.height) * imageSize.width + j;

            saliencyAccumulator -= saliency[clusterMapping[subId]];
            saliencyAccumulator += saliency[clusterMapping[addId]];
        }

        if (saliencyAccumulator > maxRegionSaliency)
        {
            maxRegionSaliency = saliencyAccumulator;
            regionCenterDistToMean = meanRegionFirstPixel - i;
            firstPixelPos.y = i;
        }
        else if (saliencyAccumulator == maxRegionSaliency)
        {
            if (abs(meanRegionFirstPixel - i) < regionCenterDistToMean)
            {
                maxRegionSaliency = saliencyAccumulator;
                regionCenterDistToMean = meanRegionFirstPixel - i;
                firstPixelPos.y = i;
            }
        }
    }
}

