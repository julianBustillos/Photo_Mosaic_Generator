#include "regionOfInterest.h"
#include "meanShift.h"
#include "saliencyFilter.h"




RegionOfInterest::RegionOfInterest(const cv::Mat & image)
{
    std::vector<int> clusterMapping;
    int nbClusters;

    MeanShift::compute(image, clusterMapping, nbClusters);
    SaliencyFilter::compute(image, clusterMapping, nbClusters);

    //TODO
    //cv::Size size = image.size();
    //_ROIBinaryImage.resize(size.width * size.height, false);
}

/*void RegionOfInterest::getROIBinaryImage(std::vector<bool>& ROIBinaryImage)
{
    ROIBinaryImage = _ROIBinaryImage;
}*/

