#include "regionOfInterest.h"
#include "MeanShift.h"






RegionOfInterest::RegionOfInterest(const cv::Mat & image)
{
    std::vector<int> clusterMapping;
    int nbClusters;
    MeanShift::compute(image, clusterMapping, nbClusters);
    

    //DEBUG
    std::vector<int> clusterVec(3 * nbClusters, 0);
    std::vector<int> clusterSize(nbClusters, 0);

    const uchar *data = image.data;
    cv::Size size = image.size();
    for (int i = 0; i < size.height; i++) {
        for (int j = 0; j < size.width; j++) {
            int currentId = i * size.width + j;
            int clusterId = clusterMapping[currentId];

            clusterVec[3 * clusterId] += data[3 * currentId];
            clusterVec[3 * clusterId + 1] += data[3 * currentId + 1];
            clusterVec[3 * clusterId + 2] += data[3 * currentId + 2];
            clusterSize[clusterId]++;
        }
    }

    for (int k = 0; k < nbClusters; k++) {
        clusterVec[3 * k] /= clusterSize[k];
        clusterVec[3 * k + 1] /= clusterSize[k];
        clusterVec[3 * k + 2] /= clusterSize[k];
    }

    cv::Mat clusterImage(size, CV_8UC3);
    uchar *clusterImageData = clusterImage.data;
    for (int i = 0; i < size.height; i++) {
        for (int j = 0; j < size.width; j++) {
            int currentId = i * size.width + j;
            int clusterId = clusterMapping[currentId];

            clusterImageData[3 * currentId] = clusterVec[3 * clusterId];
            clusterImageData[3 * currentId + 1] = clusterVec[3 * clusterId + 1];
            clusterImageData[3 * currentId + 2] = clusterVec[3 * clusterId + 2];
        }
    }

    std::cout << "Nb CLUSTERS : " << nbClusters << std::endl;

    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.push_back(0);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\Cluster.png", clusterImage, image_params);
    image_params.push_back(13);
    //DEBUG

    //TODO
    //cv::Size size = image.size();
    //_ROIBinaryImage.resize(size.width * size.height, false);
}

/*void RegionOfInterest::getROIBinaryImage(std::vector<bool>& ROIBinaryImage)
{
    ROIBinaryImage = _ROIBinaryImage;
}*/

