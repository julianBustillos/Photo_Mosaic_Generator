#include "FaceDetectionROIImpl.h"
#include "CustomException.h"
#include "MathUtils.h"
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <Windows.h>
#include <filesystem>
#include <vector>

#undef ERROR // Windows.h include issues
#undef max
#undef min


const int FaceDetectionROIImpl::_detectionSize = 640;


FaceDetectionROIImpl::FaceDetectionROIImpl()
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT); //TODO move out
    std::string processPath = getCurrentProcessDirectory(); //TODO move to init
    _faceDetector = cv::FaceDetectorYN::create(processPath + "\\ressources\\face_detection_yunet_2023mar.onnx", "", cv::Size(0, 0), 0.5, 0.3, 5000);
    if (!_faceDetector)
        throw CustomException("Bad allocation for _faceDetector in FaceDetectionROIImpl constructor.", CustomException::Level::ERROR);
}

FaceDetectionROIImpl::~FaceDetectionROIImpl()
{
    _faceDetector.reset();
}

void FaceDetectionROIImpl::find(const cv::Mat& image, cv::Rect& box, bool rowDirSearch) const
{
    //Test if face search is needed
    double croppedRatio = rowDirSearch ? (double)box.height / (double)image.size().height : (double)box.width / (double)image.size().width;

    if (croppedRatio < MinCroppedRatio)
    {
        //Deep learning based face detection using YuNet
        cv::Mat faces;
        double maxSize = std::max(image.size().width, image.size().height);
        double scale = _detectionSize / maxSize;
        double scaleInv = maxSize / _detectionSize;
        int sWidth = (int)std::round((double)image.size().width * scale);
        int sHeight = (int)std::round((double)image.size().height * scale);
        cv::Mat sImage;
        MathUtils::computeImageResampling(sImage, cv::Size(sWidth, sHeight), image, MathUtils::AREA);
        _faceDetector->setInputSize(sImage.size());
        _faceDetector->detect(sImage, faces);

        if (!faces.empty())
        {
            getDetectionROI(image.size(), faces, box, scaleInv, rowDirSearch);
            return;
        }
    }

    getDefaultROI(image.size(), box, rowDirSearch);
}

std::string FaceDetectionROIImpl::getCurrentProcessDirectory()
{
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, sizeof(buffer));
    return std::filesystem::path(buffer).parent_path().string();
}

void FaceDetectionROIImpl::getDetectionROI(const cv::Size& imageSize, const cv::Mat& faces, cv::Rect& box, double scaleInv, bool rowDirSearch) const
{
    double minConfidence = (faces.at<float>(0, 14) >= HighFaceConfidence) ? HighFaceConfidence : LowFaceConfidence; // If there is no face detected with high confidence, try other detected faces !
    cv::Point min(imageSize.width, imageSize.height);
    cv::Point max(0, 0);

    //Compute smallest ROI containing all the detected faces
    for (int i = 0; i < faces.rows; i++)
    {
        if (faces.at<float>(i, 14) < minConfidence)
        {
            break;
        }

        int x = (int)(faces.at<float>(i, 0) * scaleInv);
        int y = (int)(faces.at<float>(i, 1) * scaleInv);
        int w = (int)(faces.at<float>(i, 2) * scaleInv);
        int h = (int)(faces.at<float>(i, 3) * scaleInv);

        if (x < min.x)
        {
            min.x = x;
        }
        if (x + w > max.x)
        {
            max.x = x + w;
        }
        
        if (y < min.y)
        {
            min.y = y;
        }
        if (y + h > max.y)
        {
            max.y = y + h;
        }
    }

    //Center ROI in cropped image
    if (rowDirSearch)
    {
        box.x = 0;
        box.y = (max.y + min.y - box.height) / 2;
        if (box.y < 0)
        {
            box.y = 0;
        }
        else if (box.y + box.height > imageSize.height)
        {
            box.y = imageSize.height - box.height;
        }
    }
    else
    {
        box.x = (max.x + min.x - box.width) / 2;
        box.y = 0;
        if (box.x < 0)
        {
            box.x = 0;
        }
        else if (box.x + box.width > imageSize.width)
        {
            box.x = imageSize.width - box.width;
        }
    }
}

void FaceDetectionROIImpl::getDefaultROI(const cv::Size& imageSize, cv::Rect& box, bool rowDirSearch) const
{
    box.x = (int)(floor((imageSize.width - box.width) / 2.));
    if (rowDirSearch)
    {
        box.y = (int)(floor((imageSize.height - box.height) / 2.));
    }
    else
    {
        box.y = (int)(floor((imageSize.height - box.height) / 3.));
    }
}

void FaceDetectionROIImpl::dumpDetection(std::string path, const cv::Mat& image, const cv::Mat& faces, double scaleInv, bool confidence) const
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
        {
            cv::putText(imageCopy, cv::format("%.4f", conf), cv::Point(x, y - 5), cv::FONT_HERSHEY_DUPLEX, 1.3, red);
        }
    }

    cv::imwrite(path, imageCopy);
}
