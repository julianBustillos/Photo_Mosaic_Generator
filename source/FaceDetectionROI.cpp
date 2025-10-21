#include "FaceDetectionROI.h"
#include "CustomException.h"
#include "ImageUtils.h"
#include "SystemUtils.h"
#include "Log.h"
#include <opencv2/dnn/dnn.hpp>
#include <vector>
#include <omp.h>


const int FaceDetectionROI::_detectionSize = 640;


FaceDetectionROI::FaceDetectionROI()
{
}

FaceDetectionROI::~FaceDetectionROI()
{
    _faceDetectors.clear();
}

void FaceDetectionROI::initialize()
{
    std::string processPath = SystemUtils::getCurrentProcessDirectory();
    const int nbThreads = omp_get_max_threads();
    _faceDetectors.resize(nbThreads);
    for (int t = 0; t < nbThreads; t++)
    {
        _faceDetectors[t] = cv::FaceDetectorYN::create(processPath + "\\ressources\\face_detection_yunet_2023mar.onnx", "", cv::Size(0, 0), 0.5f, 0.3f, 5000);
        if (!_faceDetectors[t])
            throw CustomException("Bad allocation for _faceDetector in FaceDetectionROI.", CustomException::Level::ERROR);
    }
    Log::Logger::get().log(Log::TRACE) << "Face detection model loaded.";
}

void FaceDetectionROI::find(const cv::Mat& image, cv::Rect& box, bool rowDirSearch, int threadID) const
{
    //Test if face search is needed
    double croppedRatio = rowDirSearch ? (double)box.height / (double)image.rows : (double)box.width / (double)image.cols;

    if (croppedRatio < MinCroppedRatio)
    {
        //Deep learning based face detection using YuNet
        cv::Mat faces;
        double maxSize = std::max(image.cols, image.rows);
        double scale = _detectionSize / maxSize;
        double scaleInv = maxSize / _detectionSize;
        int sWidth = (int)std::round((double)image.cols * scale);
        int sHeight = (int)std::round((double)image.rows * scale);
        cv::Mat sImage;
        ImageUtils::resample(sImage, cv::Size(sWidth, sHeight), image, ImageUtils::AREA);
        _faceDetectors[threadID]->setInputSize(sImage.size());
        _faceDetectors[threadID]->detect(sImage, faces);

        if (!faces.empty())
        {
            getDetectionROI(image.size(), faces, box, scaleInv, rowDirSearch);
            return;
        }
    }

    getDefaultROI(image.size(), box, rowDirSearch);
}

void FaceDetectionROI::getDetectionROI(const cv::Size& imageSize, const cv::Mat& faces, cv::Rect& box, double scaleInv, bool rowDirSearch) const
{
    double minConfidence = (faces.at<float>(0, 14) >= HighFaceConfidence) ? HighFaceConfidence : LowFaceConfidence; // If there is no face detected with high confidence, try other detected faces !

    //Find all face boxes according to confidence
    std::vector<cv::Rect> faceBoxes;
    for (int i = 0; i < faces.rows; i++)
    {
        if (faces.at<float>(i, 14) < minConfidence)
            break;

        int x = (int)(faces.at<float>(i, 0) * scaleInv);
        int y = (int)(faces.at<float>(i, 1) * scaleInv);
        int w = (int)(faces.at<float>(i, 2) * scaleInv);
        int h = (int)(faces.at<float>(i, 3) * scaleInv);
        faceBoxes.emplace_back(x, y, w, h);
    }

    //Sort face boxes according to image center proximity
    std::sort(faceBoxes.begin(), faceBoxes.end(), [&](const cv::Rect& lhs, const cv::Rect& rhs)
              {
                  double imageCenter, leftFaceCenter, rightFaceCenter;
                  if (rowDirSearch)
                  {
                      imageCenter = (double)imageSize.height * 0.5;
                      leftFaceCenter = (double)lhs.y + (double)lhs.height * 0.5;
                      rightFaceCenter = (double)rhs.y + (double)rhs.height * 0.5;
                  }
                  else
                  {
                      imageCenter = (double)imageSize.width * 0.5;
                      leftFaceCenter = (double)lhs.x + (double)lhs.width * 0.5;
                      rightFaceCenter = (double)rhs.x + (double)rhs.width * 0.5;
                  }

                  return abs(imageCenter - leftFaceCenter) < abs(imageCenter - rightFaceCenter);
              });

    //Find ROI with optimal number of faces
    for (int nbFaces = faceBoxes.size(); nbFaces > 0; nbFaces--)
    {
        //Compute smallest ROI containing all the detected faces
        cv::Point min(imageSize.width, imageSize.height);
        cv::Point max(0, 0);
        for (int i = 0; i < nbFaces; i++)
        {
            if (faceBoxes[i].x < min.x)
                min.x = faceBoxes[i].x;
            if (faceBoxes[i].x + faceBoxes[i].width > max.x)
                max.x = faceBoxes[i].x + faceBoxes[i].width;

            if (faceBoxes[i].y < min.y)
                min.y = faceBoxes[i].y;
            if (faceBoxes[i].y + faceBoxes[i].height > max.y)
                max.y = faceBoxes[i].y + faceBoxes[i].height;
        }

        //Center ROI in cropped image
        if (rowDirSearch)
        {
            if (faceBoxes.size() == 1 || max.y - min.y <= box.height * (1. + FaceBoxTolerance))
            {
                box.x = 0;
                box.y = (max.y + min.y - box.height) / 2;
                if (box.y < 0)
                    box.y = 0;
                else if (box.y + box.height > imageSize.height)
                    box.y = imageSize.height - box.height;
                break;
            }
        }
        else
        {
            if (faceBoxes.size() == 1 || max.x - min.x <= box.width * (1. + FaceBoxTolerance))
            {
                box.x = (max.x + min.x - box.width) / 2;
                box.y = 0;
                if (box.x < 0)
                    box.x = 0;
                else if (box.x + box.width > imageSize.width)
                    box.x = imageSize.width - box.width;
                break;
            }
        }
    }
}

void FaceDetectionROI::getDefaultROI(const cv::Size& imageSize, cv::Rect& box, bool rowDirSearch) const
{
    box.x = (int)(floor((imageSize.width - box.width) / 2.));
    if (rowDirSearch)
        box.y = (int)(floor((imageSize.height - box.height) / 2.));
    else
        box.y = (int)(floor((imageSize.height - box.height) / 3.));
}

void FaceDetectionROI::dumpDetection(std::string path, const cv::Mat& image, const cv::Mat& faces, double scaleInv, bool confidence) const
{
    cv::Scalar red(50, 50, 255);
    cv::Mat imageCopy(image);
    for (int i = 0; i < faces.rows; i++)
    {
        int x = (int)(faces.at<float>(i, 0) * scaleInv);
        int y = (int)(faces.at<float>(i, 1) * scaleInv);
        int w = (int)(faces.at<float>(i, 2) * scaleInv);
        int h = (int)(faces.at<float>(i, 3) * scaleInv);
        float conf = faces.at<float>(i, 14);
        cv::Rect rect(x, y, w, h);
        cv::rectangle(imageCopy, rect, red, 3);
        if (confidence)
            cv::putText(imageCopy, cv::format("%.4f", conf), cv::Point(x, y - 5), cv::FONT_HERSHEY_DUPLEX, 1.3, red);
    }

    cv::imwrite(path, imageCopy);
}
