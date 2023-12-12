#include "SaliencyFilter.h"
#include "Utils.h"
#include "variables.h"
#include <limits>


const double SaliencyFilter::uniquenessCoefficient = 1. / (2. * SALIENCY_FILTER_UNIQUENESS_SIGMA * SALIENCY_FILTER_UNIQUENESS_SIGMA);
const double SaliencyFilter::distributionCoefficient = 1. / (2. * SALIENCY_FILTER_DISTRIBUTION_SIGMA * SALIENCY_FILTER_DISTRIBUTION_SIGMA);


void SaliencyFilter::compute(const cv::Mat& image, const std::vector<int>& clusterMapping, int nbClusters, std::vector<double>& saliency, int& iMean, int& jMean, bool& saliencyFound)
{
    std::vector<ClusterPoint> cluster(nbClusters);
    buildClusters(cluster, image, clusterMapping);

    std::vector<double> colorSqDistance(nbClusters * nbClusters, 0.);
    computeColorSqDistance(cluster, colorSqDistance);

    computeUniqueness(colorSqDistance, cluster);
    computeDistribution(colorSqDistance, cluster);
    computeSaliency(cluster);

    double threshold;
    findSaliencyThreshold(cluster, threshold);
    findMeanClusterPoint(cluster, threshold, iMean, jMean);
    buildThresholdedSaliency(cluster, saliency, threshold);
    saliencyFound = threshold < 1.;

    //DEBUG
    /*cv::Mat clusterImage(image.size(), CV_8UC3);
    uchar *clusterImageData = clusterImage.data;
    std::vector<uchar> clusterVec(3 * nbClusters);
    for (int k = 0; k < nbClusters; k++) {
        MathTools::convertLABtoBGR(clusterVec[3 * k], clusterVec[3 * k + 1], clusterVec[3 * k + 2], cluster[k]._L, cluster[k]._a, cluster[k]._b);
    }

    for (int i = 0; i < image.size().height; i++) {
        for (int j = 0; j < image.size().width; j++) {
            int currentId = i * image.size().width + j;
            int clusterId = clusterMapping[currentId];

            clusterImageData[3 * currentId] = clusterVec[3 * clusterId];
            clusterImageData[3 * currentId + 1] = clusterVec[3 * clusterId + 1];
            clusterImageData[3 * currentId + 2] = clusterVec[3 * clusterId + 2];
        }
    }

    cv::Mat uniquenessImage(image.size(), CV_8UC3);
    uchar *uniquenessImageData = uniquenessImage.data;
    for (int i = 0; i < image.size().height; i++) {
        for (int j = 0; j < image.size().width; j++) {
            int currentId = i * image.size().width + j;
            int clusterId = clusterMapping[currentId];

            uniquenessImageData[3 * currentId] = (uchar)std::round(cluster[clusterId]._uniqueness * 255.);
            uniquenessImageData[3 * currentId + 1] = (uchar)std::round(cluster[clusterId]._uniqueness * 255.);
            uniquenessImageData[3 * currentId + 2] = (uchar)std::round(cluster[clusterId]._uniqueness * 255.);
        }
    }

    cv::Mat distributionImage(image.size(), CV_8UC3);
    uchar *distributionImageData = distributionImage.data;
    for (int i = 0; i < image.size().height; i++) {
        for (int j = 0; j < image.size().width; j++) {
            int currentId = i * image.size().width + j;
            int clusterId = clusterMapping[currentId];

            distributionImageData[3 * currentId] = (uchar)std::round(cluster[clusterId]._distribution * 255.);
            distributionImageData[3 * currentId + 1] = (uchar)std::round(cluster[clusterId]._distribution * 255.);
            distributionImageData[3 * currentId + 2] = (uchar)std::round(cluster[clusterId]._distribution * 255.);
        }
    }

    cv::Mat saliencyImage(image.size(), CV_8UC3);
    uchar *saliencyImageData = saliencyImage.data;
    for (int i = 0; i < image.size().height; i++) {
        for (int j = 0; j < image.size().width; j++) {
            int currentId = i * image.size().width + j;
            int clusterId = clusterMapping[currentId];

            saliencyImageData[3 * currentId] = (uchar)std::round(cluster[clusterId]._saliency * 255.);
            saliencyImageData[3 * currentId + 1] = (uchar)std::round(cluster[clusterId]._saliency * 255.);
            saliencyImageData[3 * currentId + 2] = (uchar)std::round(cluster[clusterId]._saliency * 255.);
        }
    }

    cv::Mat saliencyKeptImage(image.size(), CV_8UC3);
    uchar *saliencyKeptImageData = saliencyKeptImage.data;
    for (int i = 0; i < image.size().height; i++) {
        for (int j = 0; j < image.size().width; j++) {
            int currentId = i * image.size().width + j;
            int clusterId = clusterMapping[currentId];

            bool saliencyKept = (cluster[clusterId]._saliency > SALIENCY_MIN_THRESHOLD);

            saliencyKeptImageData[3 * currentId] = saliencyKept ? image.data[3 * currentId] : 0;
            saliencyKeptImageData[3 * currentId + 1] = saliencyKept ? image.data[3 * currentId + 1] : 0;
            saliencyKeptImageData[3 * currentId + 2] = saliencyKept ? image.data[3 * currentId + 2] : 0;
        }
    }

    std::cout << "Nb CLUSTERS : " << nbClusters << std::endl;
    std::vector<int> image_params;
    image_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    image_params.push_back(0);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_0_Original.png", image, image_params);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_1_Cluster.png", clusterImage, image_params);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_2_Uniqueness.png", uniquenessImage, image_params);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_3_Distribution.png", distributionImage, image_params);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_4_Saliency.png", saliencyImage, image_params);
    cv::imwrite("C:\\Users\\Julian Bustillos\\Downloads\\MOSAIC_TEST\\DEBUG_5_Saliency_kept.png", saliencyKeptImage, image_params);
    image_params.push_back(13);
    std::exit(EXIT_SUCCESS);*/
    //DEBUG
}

void SaliencyFilter::buildClusters(std::vector<ClusterPoint>& cluster, const cv::Mat& image, const std::vector<int>& clusterMapping)
{
    const uchar* data = image.data;
    cv::Size size = image.size();

    for (int i = 0; i < size.height; i++)
    {
        for (int j = 0; j < size.width; j++)
        {
            int currentId = i * size.width + j;
            int clusterId = clusterMapping[currentId];

            cluster[clusterId]._i += i;
            cluster[clusterId]._j += j;
            cluster[clusterId]._B += data[3 * currentId];
            cluster[clusterId]._G += data[3 * currentId + 1];
            cluster[clusterId]._R += data[3 * currentId + 2];
            cluster[clusterId]._size++;
        }
    }

    for (int clusterId = 0; clusterId < cluster.size(); clusterId++)
    {
        cluster[clusterId]._i /= cluster[clusterId]._size;
        cluster[clusterId]._j /= cluster[clusterId]._size;
        cluster[clusterId]._B = (int)std::round((double)cluster[clusterId]._B / (double)cluster[clusterId]._size);
        cluster[clusterId]._G = (int)std::round((double)cluster[clusterId]._G / (double)cluster[clusterId]._size);
        cluster[clusterId]._R = (int)std::round((double)cluster[clusterId]._R / (double)cluster[clusterId]._size);
        Utils::convertBGRtoLAB(cluster[clusterId]._L, cluster[clusterId]._a, cluster[clusterId]._b, (uchar)cluster[clusterId]._B, (uchar)cluster[clusterId]._G, (uchar)cluster[clusterId]._R);
    }
}

void SaliencyFilter::computeColorSqDistance(const std::vector<ClusterPoint>& cluster, std::vector<double>& colorSqDistance)
{
    int size = (int)cluster.size();
    for (int i = 0; i < size; i++)
    {
        for (int j = i + 1; j < size; j++)
        {
            int id = i * size + j;
            int idSym = j * size + i;

            double dL = cluster[i]._L - cluster[j]._L;
            double da = cluster[i]._a - cluster[j]._a;
            double db = cluster[i]._b - cluster[j]._b;

            colorSqDistance[id] = colorSqDistance[idSym] = dL * dL + da * da + db * db;
        }
    }
}

void SaliencyFilter::computeUniqueness(const std::vector<double>& colorSqDistance, std::vector<ClusterPoint>& cluster)
{
    int size = (int)cluster.size();
    std::vector<double> weight(size * size, 1.);

    for (int i = 0; i < size; i++)
    {
        for (int j = i + 1; j < size; j++)
        {
            int id = i * size + j;
            int idSym = j * size + i;

            double di = cluster[i]._i - cluster[j]._i;
            double dj = cluster[i]._j - cluster[j]._j;

            weight[id] = weight[idSym] = std::exp(-uniquenessCoefficient * (di * di + dj * dj));
        }
    }

    double maxUniqueness = 0., minUniqueness = std::numeric_limits<double>::max();
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        double weightSum = 0.;
        for (int compareId = 0; compareId < size; compareId++)
        {
            int currentId = clusterId * size + compareId;
            double currentWeight = cluster[compareId]._size * weight[currentId];

            cluster[clusterId]._uniqueness += colorSqDistance[currentId] * currentWeight;
            weightSum += currentWeight;
        }
        cluster[clusterId]._uniqueness /= weightSum;

        if (cluster[clusterId]._uniqueness > maxUniqueness)
            maxUniqueness = cluster[clusterId]._uniqueness;
        else if (cluster[clusterId]._uniqueness < minUniqueness)
            minUniqueness = cluster[clusterId]._uniqueness;
    }

    for (int clusterId = 0; clusterId < size; clusterId++)
        cluster[clusterId]._uniqueness = (cluster[clusterId]._uniqueness - minUniqueness) / (maxUniqueness - minUniqueness);
}

void SaliencyFilter::computeDistribution(const std::vector<double>& colorSqDistance, std::vector<ClusterPoint>& cluster)
{
    int size = (int)cluster.size();
    std::vector<double> weight(size * size, 1.);

    for (int i = 0; i < size; i++)
    {
        for (int j = i + 1; j < size; j++)
        {
            int id = i * size + j;
            int idSym = j * size + i;

            weight[id] = weight[idSym] = std::exp(-distributionCoefficient * colorSqDistance[id]);
        }
    }

    double maxDistribution = 0., minDistribution = std::numeric_limits<double>::max();
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        double weightSum = 0.;
        double mui = 0., muj = 0.;
        for (int compareId = 0; compareId < size; compareId++)
        {
            int currentId = clusterId * size + compareId;
            double currentWeight = cluster[compareId]._size * weight[currentId];

            mui += cluster[compareId]._i * currentWeight;
            muj += cluster[compareId]._j * currentWeight;
            cluster[clusterId]._distribution += (cluster[compareId]._i * cluster[compareId]._i + cluster[compareId]._j * cluster[compareId]._j) * currentWeight;
            weightSum += currentWeight;
        }
        cluster[clusterId]._distribution /= weightSum;
        cluster[clusterId]._distribution -= (mui * mui + muj * muj) / (weightSum * weightSum);

        if (cluster[clusterId]._distribution > maxDistribution)
            maxDistribution = cluster[clusterId]._distribution;
        else if (cluster[clusterId]._distribution < minDistribution)
            minDistribution = cluster[clusterId]._distribution;
    }

    for (int clusterId = 0; clusterId < size; clusterId++)
        cluster[clusterId]._distribution = (cluster[clusterId]._distribution - minDistribution) / (maxDistribution - minDistribution);
}

void SaliencyFilter::computeSaliency(std::vector<ClusterPoint>& cluster)
{
    int size = (int)cluster.size();

    double maxSaliency = 0.;
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        cluster[clusterId]._saliency = cluster[clusterId]._uniqueness * std::exp(-SALIENCY_DISTRIBUTION_INFLUENCE * cluster[clusterId]._distribution);
        if (cluster[clusterId]._saliency > maxSaliency)
            maxSaliency = cluster[clusterId]._saliency;
    }

    for (int clusterId = 0; clusterId < size; clusterId++)
        cluster[clusterId]._saliency /= maxSaliency;
}

void SaliencyFilter::findSaliencyThreshold(const std::vector<ClusterPoint>& cluster, double& threshold)
{
    int size = (int)cluster.size();
    std::vector<std::pair<double, int>> sortedSaliency(size);

    int nbPixel = 0;
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        sortedSaliency[clusterId].first = cluster[clusterId]._saliency;
        sortedSaliency[clusterId].second = cluster[clusterId]._size;
        nbPixel += cluster[clusterId]._size;
    }

    std::sort(sortedSaliency.begin(), sortedSaliency.end());

    int pixelSum = 0;
    const int minZeroSaliencyRegionSize = (int)(nbPixel * (1. - SALIENCY_REGION_MAX_SIZE));
    threshold = 0.;
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        pixelSum += sortedSaliency[clusterId].second;
        if (pixelSum > minZeroSaliencyRegionSize)
        {
            threshold = sortedSaliency[clusterId].first;
            break;
        }
    }

    if (threshold < SALIENCY_MIN_THRESHOLD)
        threshold = SALIENCY_MIN_THRESHOLD;
}

void SaliencyFilter::findMeanClusterPoint(const std::vector<ClusterPoint>& cluster, double threshold, int& iMean, int& jMean)
{
    int size = (int)cluster.size();
    double iSum = 0., jSum = 0., weightSum = 0.;

    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        double currentWeight = (cluster[clusterId]._saliency > threshold)?cluster[clusterId]._saliency > threshold * cluster[clusterId]._size : 0;
        iSum += cluster[clusterId]._i * currentWeight;
        jSum += cluster[clusterId]._j * currentWeight;
        weightSum += currentWeight;
    }

    iMean = (int)(iSum / weightSum + 0.5);
    jMean = (int)(jSum / weightSum + 0.5);
}

void SaliencyFilter::buildThresholdedSaliency(const std::vector<ClusterPoint>& cluster, std::vector<double>& saliency, double threshold)
{
    int size = (int)cluster.size();
    saliency.resize(size);
    for (int clusterId = 0; clusterId < size; clusterId++)
    {
        saliency[clusterId] = (cluster[clusterId]._saliency > threshold)?cluster[clusterId]._saliency:0.;
    }
}
