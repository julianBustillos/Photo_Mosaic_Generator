#include "ImageUtils.h"
#include "ColorUtils.h"
#include <numbers>


namespace
{
    double sinc(double x)
    {
        if (x == 0.0)
        {
            return 1.0;
        }
        x = x * std::numbers::pi;
        return sin(x) / x;
    }

    class SamplingFilter
    {
    public:
        SamplingFilter(double support) : _support(support), _scale(0), _invScale(0) {};
        void setScale(double scale) { _scale = scale > 1. ? scale : 1.; _invScale = 1. / _scale; };
        double getSupport() const { return _support * _scale; };
        virtual double compute(double xPos, double center) const = 0;

    protected:
        const double _support;
        double _scale;
        double _invScale;
    };

    class AreaFilter : public SamplingFilter
    {
    public:
        AreaFilter() : SamplingFilter(1.) {};
        virtual double compute(double xPos, double center) const
        {
            double minVal = center - _scale * 0.5;
            double maxVal = center + _scale * 0.5;
            double minPos = std::ceil(minVal);
            double maxPos = std::floor(maxVal);

            if (minPos <= xPos && xPos <= maxPos)
            {
                return 1.0;
            }
            else if (minPos - 1. <= xPos && xPos <= minPos)
            {
                return minPos - minVal;
            }
            else if (maxPos <= xPos && xPos <= maxPos + 1.)
            {
                return maxVal - maxPos;
            }
            return 0.0;
        }
    };

    class BicubicFilter : public SamplingFilter
    {
    public:
        BicubicFilter() : SamplingFilter(2.) {};
        virtual double compute(double xPos, double center) const
        {
            constexpr double a = -0.5;

            double x = (xPos - center) * _invScale;
            if (x < 0.0)
            {
                x = -x;
            }
            if (x < 1.0)
            {
                return ((a + 2.0) * x - (a + 3.0)) * x * x + 1;
            }
            if (x < 2.0)
            {
                return (((x - 5) * x + 8) * x - 4) * a;
            }
            return 0.0;
        }
    };

    class LanczosFilter : public SamplingFilter
    {
    public:
        LanczosFilter() : SamplingFilter(3.) {};
        virtual double compute(double xPos, double center) const
        {
            constexpr double a = 3.0;

            double x = (xPos - center) * _invScale;
            if (-a <= x && x < a)
            {
                return sinc(x) * sinc(x / a);
            }
            return 0.0;
        }
    };

    void computeGrayscale(cv::Mat& target, const cv::Mat& source)
    {
        constexpr double HalfPixel = 0.5;
        const int nbPixels = source.rows * source.cols;
        target.create(source.size(), CV_8UC1);

        for (int pt = 0, ps = 0; pt < nbPixels; pt++, ps += 3)
        {
            target.data[pt] = (uchar)(source.data[ps] * 0.114 + source.data[ps + 1] * 0.587 + source.data[ps + 2] * 0.299 + HalfPixel);
        }
    }

    void computeCoefficients(int inSize, int outSize, int min, int max, int& nbCoeffs, std::vector<double>& coeffs, std::vector<int>& bounds, SamplingFilter* filter)
    {
        const double scale = (double)(max - min) / (double)outSize;
        filter->setScale(scale);
        const double support = filter->getSupport();
        nbCoeffs = (int)std::ceil(support) * 2 + 1;

        coeffs.resize(outSize * nbCoeffs);
        bounds.resize(outSize * 2);

        constexpr double HalfPixel = 0.5;
        for (int xOut = 0; xOut < outSize; xOut++)
        {
            double center = min + (xOut + HalfPixel) * scale;

            int xMin = (int)(center - support + HalfPixel);
            if (xMin < 0)
                xMin = 0;

            int xMax = (int)(center + support + HalfPixel);
            if (xMax > inSize)
                xMax = inSize;

            double* coeff = &coeffs[xOut * nbCoeffs];

            int xSize = xMax - xMin;
            double sum = 0.;
            for (int x = 0; x < xSize; x++)
            {
                coeff[x] = filter->compute(x + xMin + HalfPixel, center);
                sum += coeff[x];
            }

            if (sum != 0.)
            {
                double invSum = 1. / sum;
                for (int x = 0; x < xSize; x++)
                {
                    coeff[x] *= invSum;
                }
            }

            for (int x = xSize; x < nbCoeffs; x++)
            {
                coeff[x] = 0;
            }

            bounds[xOut * 2 + 0] = xMin;
            bounds[xOut * 2 + 1] = xSize;
        }
    }

    void horizontalResampling(cv::Mat& output, const cv::Mat& input, int offset, int nbCoeffs, const std::vector<double>& coeffs, const std::vector<int> bounds)
    {
        constexpr unsigned int PrecisionBits = 32 - 8 - 2;
        constexpr double PrecisionShift = (double)(1U << PrecisionBits);
        constexpr double PixelInit = 1U << (PrecisionBits - 1U);

        const int coeffsSize = coeffs.size();
        std::vector<double> shiftedCoeffs;
        shiftedCoeffs.resize(coeffsSize);

        for (int c = 0; c < coeffsSize; c++)
        {
            shiftedCoeffs[c] = std::round(coeffs[c] * PrecisionShift);
        }

        const int outHeight = output.rows;
        const int outWidth = output.cols;
        const int inWidth = input.cols;
        const int channels = input.channels();

        int pOut = 0;
        for (int yOut = 0; yOut < outHeight; yOut++)
        {
            for (int xOut = 0; xOut < outWidth; xOut++, pOut += channels)
            {
                const int xMin = bounds[xOut * 2 + 0];
                const int xSize = bounds[xOut * 2 + 1];
                const double* coeff = &shiftedCoeffs[xOut * nbCoeffs];
                int pIn = ((yOut + offset) * inWidth + xMin) * channels;
                for (int c = 0; c < channels; c++)
                {
                    double pixel = PixelInit;
                    for (int x = 0, p = pIn + c; x < xSize; x++, p += channels)
                    {
                        pixel += (double)input.data[p] * coeff[x];
                    }
                    output.data[pOut + c] = (uchar)ColorUtils::clip<int>((int)pixel >> PrecisionBits, 0, 255);
                }
            }
        }
    }

    void verticalResampling(cv::Mat& output, const cv::Mat& input, int nbCoeffs, const std::vector<double>& coeffs, const std::vector<int> bounds)
    {
        output = output.t();
        horizontalResampling(output, input.t(), 0, nbCoeffs, coeffs, bounds);
        output = output.t();
    }

    void copy(cv::Mat& output, const cv::Mat& input, const cv::Rect& box)
    {
        const int height = output.rows;
        const int width = output.cols;
        const int channels = output.channels();
        const int step = (input.cols - box.width) * channels;

        int pOut = 0;
        int pIn = (box.y * input.cols + box.x) * channels;
        for (int y = 0; y < height; y++, pIn += step)
        {
            for (int x = 0; x < width; x++)
            {
                for (int c = 0; c < channels; c++, pOut++, pIn++)
                {
                    output.data[pOut] = input.data[pIn];
                }
            }
        }
    }

    void computeResampling(cv::Mat& target, const cv::Size& targetSize, const cv::Mat& source, const cv::Rect& box, SamplingFilter* filter)
    {
        cv::Mat temp;
        const bool doHoriSampling = targetSize.width != box.width;
        const bool doVertSampling = targetSize.height != box.height;

        cv::Vec4i limits(box.x, box.y, box.x + box.width, box.y + box.height);

        int nbCoeffsHori = 0, nbCoeffsVert = 0;
        std::vector<double> coeffsHori, coeffsVert;
        std::vector<int> boundsHori, boundsVert;

        if (doHoriSampling)
            computeCoefficients(source.cols, targetSize.width, limits[0], limits[2], nbCoeffsHori, coeffsHori, boundsHori, filter);

        if (doVertSampling)
            computeCoefficients(source.rows, targetSize.height, limits[1], limits[3], nbCoeffsVert, coeffsVert, boundsVert, filter);

        if (doHoriSampling)
        {
            const int yLowBound = boundsVert[0];
            const int yHighBound = boundsVert[targetSize.height * 2 - 2] + boundsVert[targetSize.height * 2 - 1];

            for (int i = 0; i < targetSize.height; ++i)
                boundsVert[i * 2] -= yLowBound;

            cv::Mat& output = doVertSampling ? temp : target;
            output.create(yHighBound - yLowBound, targetSize.width, source.type());
            horizontalResampling(output, source, yLowBound, nbCoeffsHori, coeffsHori, boundsHori);
        }

        if (doVertSampling)
        {
            const cv::Mat& input = doHoriSampling ? temp : source;
            target.create(targetSize, source.type());
            verticalResampling(target, input, nbCoeffsVert, coeffsVert, boundsVert);
        }

        if (!doHoriSampling && !doVertSampling)
        {
            target.create(targetSize, source.type());
            copy(target, source, box);
        }
    }

    void getGaussianApproxBoxRadiuses(double sigma, int* boxRadius)
    {
        int n = ImageUtils::BlurNbBoxes;
        double wIdeal = sqrt(12. * sigma * sigma / n + 1.);
        int wl = (int)floor(wIdeal);
        if ((wl % 2) == 0)
            wl--;
        int wu = wl + 2;
        int m = (int)round((12. * sigma * sigma - n * wl * wl - 4. * n * wl - 3 * n) / (-4. * wl - 4.));
        int wlr = (wl - 1) / 2;
        int wur = (wu - 1) / 2;

        for (int k = 0; k < n; k++)
            boxRadius[k] = (k < m) ? wlr : wur;
    }

    void applyRowBlur(uchar* source, uchar* target, const cv::Size& size, int lineRadius)
    {
        double invLineSize = 1. / (double)(2 * lineRadius + 1);
        int accumulator[3];
        int start[3];
        int end[3];

        for (int i = 0; i < size.height; i++)
        {
            // Average filter start middle and end indices
            int startId = 3 * i * size.width;
            int endId = 3 * (i * size.width + lineRadius);
            int targetId = startId;

            // Initialization 
            for (int c = 0; c < 3; c++)
            {
                start[c] = source[startId + c];
                end[c] = source[3 * ((i + 1) * size.width - 1) + c];
                accumulator[c] = start[c] * (lineRadius + 1);
            }

            for (int j = 0; j < lineRadius; j++)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[startId + 3 * j + c];
                }
            }

            // Average filtering
            for (int j = 0; j <= lineRadius; j++, targetId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[endId + c] - start[c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }

            for (int j = lineRadius + 1; j < size.width - lineRadius; j++, targetId += 3, startId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += source[endId + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }

            for (int j = size.width - lineRadius; j < size.width; j++, targetId += 3, startId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[c] += end[c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[c] * invLineSize + 0.5);
                }
            }
        }
    }

    void applyColBlur(uchar* source, uchar* target, const cv::Size& size, int lineRadius)
    {
        double invLineSize = 1. / (double)(2 * lineRadius + 1);
        std::vector<int> accumulator(3 * size.width);
        std::vector<int> start(3 * size.width);
        std::vector<int> end(3 * size.width);

        // Average filter start middle and end indices
        int startId = 0;
        int endId = 3 * (lineRadius * size.width);
        int targetId = 0;

        // Initialization 
        for (int j = 0; j < size.width; j++)
        {
            for (int c = 0; c < 3; c++)
            {
                start[3 * j + c] = source[3 * j + c];
                end[3 * j + c] = source[3 * ((size.height - 1) * size.width + j) + c];
                accumulator[3 * j + c] = start[3 * j + c] * (lineRadius + 1);
            }
        }

        for (int i = 0; i < lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[3 * (i * size.width + j) + c];
                }
            }
        }

        // Average filtering
        for (int i = 0; i <= lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[endId + c] - start[3 * j + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }

        for (int i = lineRadius + 1; i < size.height - lineRadius; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, startId += 3, endId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += source[endId + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }

        for (int i = size.height - lineRadius; i < size.height; i++)
        {
            for (int j = 0; j < size.width; j++, targetId += 3, startId += 3)
            {
                for (int c = 0; c < 3; c++)
                {
                    accumulator[3 * j + c] += end[3 * j + c] - source[startId + c];
                    target[targetId + c] = (uchar)(accumulator[3 * j + c] * invLineSize + 0.5);
                }
            }
        }
    }

    void applyBoxBlur(uchar* image, uchar* buffer, const cv::Size& size, int boxRadius)
    {
        applyRowBlur(image, buffer, size, boxRadius);
        applyColBlur(buffer, image, size, boxRadius);
    }

    void computeKernel(std::vector<int>& kernel, const cv::Size& size, int radius)
    {
        kernel.resize(size.width * size.height);

        for (int i = 0, k = 0; i < size.height; i++)
        {
            int iSize = 1;
            iSize += (i >= radius) ? radius : i;
            iSize += (size.height - i - 1 >= radius) ? radius : size.height - i - 1;

            for (int j = 0; j < size.width; j++, k++)
            {
                int jSize = 1;
                jSize += (j >= radius) ? radius : j;
                jSize += (size.width - j - 1 >= radius) ? radius : size.width - j - 1;

                kernel[k] = iSize * jSize;
            }
        }
    }

    void averageFilter(std::vector<double>& output, std::vector<double>& buffer, const std::vector<double>& input, const std::vector<int>& kernel, const cv::Size& size, int radius)
    {
        std::fill(buffer.begin(), buffer.end(), 0.);
        const int b0 = (size.width + 2 * radius + 2) * (radius + 1) * 3;

        //Initialize buffer
        int bs = (2 * radius + 1) * 3;
        for (int i = 0, k = 0, b = b0; i < size.height; i++, b += bs)
            for (int j = 0; j < size.width * 3; j++, k++, b++)
                buffer[b] = input[k];
        
        const int bWidth = (size.width + 2 * radius + 1) * 3;
        const int bHeight = size.height + 2 * radius + 1;

        //Integral image computation
        bs = (radius + 1) * 3;
        for (int i = radius + 1, b = b0; i < bHeight; i++, b += bs)
            for (int j = (radius + 1) * 3; j < bWidth; j++, b++)
                buffer[b] += buffer[b - bWidth];

        for (int i = radius + 1, b = b0; i < bHeight; i++, b += bs)
            for (int j = (radius + 1) * 3; j < bWidth; j++, b++)
                buffer[b] += buffer[b - 3];

        const int ws = (2 * radius + 1) * 3;
        int w0 = 0;
        int w1 = ws;
        int w2 = (size.width + 2 * radius + 1) * (2 * radius + 1) * 3;
        int w3 = w2 + ws;
        for (int i = 0, k = 0; i < size.height; i++, w0 += ws, w1 += ws, w2 += ws, w3 += ws)
            for (int j = 0; j < size.width * 3; j++, k++, w0++, w1++, w2++, w3++)
                output[k] = (buffer[w0] + buffer[w3] - buffer[w1] - buffer[w2]) / kernel[k / 3];
    }
};


void ImageUtils::resample(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, Filter filter)
{
    cv::Rect box(0, 0, source.cols, source.rows);
    resample(target, targetSize, source, box, filter);
}

void ImageUtils::resample(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Rect& box, Filter filter)
{
    std::unique_ptr<SamplingFilter> samplingFilter;

    if (filter == ImageUtils::AREA)
    {
        samplingFilter = std::make_unique<AreaFilter>();
    }
    else if (filter == ImageUtils::BICUBIC)
    {
        samplingFilter = std::make_unique<BicubicFilter>();
    }
    else if (filter == ImageUtils::LANCZOS)
    {
        samplingFilter = std::make_unique<LanczosFilter>();
    }

    if (samplingFilter)
        computeResampling(target, targetSize, source, box, samplingFilter.get());
}

void ImageUtils::computeFeatures(const cv::Mat& image, double* features, int featureDiv, int nbFeatures)
{
    const int width = image.cols;
    const int height = image.rows;
    int blockWidth = (int)ceil(width / (double)featureDiv);
    int blockHeight = (int)ceil(height / (double)featureDiv);

    for (int k = 0; k < nbFeatures; k++)
        features[k] = 0;

    for (int i = 0, p = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int blockPos = 3 * (featureDiv * (i / blockHeight) + j / blockWidth);
            for (int c = 0; c < 3; c++, p++)
            {
                features[blockPos + c] += image.data[p];
            }
        }
    }

    for (int k = 0; k < nbFeatures; k++)
    {
        int corrBlockHeight = (k < featureDiv * (featureDiv - 1) * 3) ? blockHeight : height - (featureDiv - 1) * blockHeight;
        int corrBlockWidth = (((k / 4 + 1) % featureDiv) != 0) ? blockWidth : width - (featureDiv - 1) * blockWidth;
        features[k] /= corrBlockWidth * corrBlockHeight;
    }
}

double ImageUtils::featureDistance(const double* features1, const double* features2, int nbFeatures)
{
    //Use deltaE distance
    double sumDist = 0.;
    for (int i = 0; i < nbFeatures; i += 3)
    {
        double dB = features1[i] - features2[i];
        double dG = features1[i + 1] - features2[i + 1];
        double dR = features1[i + 2] - features2[i + 2];
        double mR = (features1[i + 2] + features2[i + 2]) * 0.5;
        double sqDist = (2. + mR / 256.) * dR * dR + 4 * dG * dG + (2. + (255. - mR) / 256.) * dB * dB;
        sumDist += sqrt(sqDist);
    }

    return sumDist;
}

void ImageUtils::gaussianBlur(uchar* image, const cv::Size& size, double sigma)
{
    std::vector<uchar> buffer(3 * size.width * size.height);

    int boxRadius[BlurNbBoxes];
    getGaussianApproxBoxRadiuses(sigma, boxRadius);

    if (boxRadius[BlurNbBoxes - 1] <= std::min(size.width, size.height) / 2)
    {
        for (int k = 0; k < BlurNbBoxes; k++)
            applyBoxBlur(image, buffer.data(), size, boxRadius[k]);
    }
}

void ImageUtils::DHash(const cv::Mat& image, Hash& hash)
{
    int hashId = 0;
    cv::Mat grayscale, hashImage;
    cv::Size dhashHoriSize(HashSize + 1, HashSize);
    cv::Size dhashVertSize(HashSize, HashSize + 1);

    computeGrayscale(grayscale, image);

    resample(hashImage, dhashHoriSize, grayscale, LANCZOS);
    int pH = 0;
    for (int i = 0; i < dhashHoriSize.height; i++, pH++)
    {
        for (int j = 0; j < dhashHoriSize.width - 1; j++, hashId++, pH++)
        {
            if (hashImage.data[pH] < hashImage.data[pH + 1])
            {
                hash.set(hashId);
            }
        }
    }

    resample(hashImage, dhashVertSize, grayscale, LANCZOS);
    int pV = 0;
    for (int i = 0; i < dhashVertSize.height - 1; i++)
    {
        for (int j = 0; j < dhashVertSize.width; j++, hashId++, pV++)
        {
            if (hashImage.data[pV] < hashImage.data[pV + dhashVertSize.width])
            {
                hash.set(hashId);
            }
        }
    }
}

void ImageUtils::guidedFiltering(std::vector<double>& filtered, const std::vector<double>& image, const std::vector<double>& guide, const cv::Size& size, int radius, double epsilon)
{
    std::vector<int> kernel;
    std::vector<double> buffer((size.width + 2 * radius + 1) * (size.height + 2 * radius + 1) * 3);
    int imageSize = size.width * size.height * 3;
    std::vector<double> sqGuide(imageSize);
    std::vector<double> imgGuide(imageSize);
    std::vector<double> guideVar(imageSize);
    std::vector<double> imgGuideCov(imageSize);
    std::vector<double> alpha(imageSize);
    std::vector<double> beta(imageSize);
    std::vector<double> imgMean(imageSize);
    std::vector<double> guideMean(imageSize);
    std::vector<double> guideCorr(imageSize);
    std::vector<double> imgGuideCorr(imageSize);
    std::vector<double> alphaMean(imageSize);
    std::vector<double> betaMean(imageSize);

    for (int i = 0; i < sqGuide.size(); i++)
        sqGuide[i] = guide[i] * guide[i];

    for (int i = 0; i < imgGuide.size(); i++)
        imgGuide[i] = image[i] * guide[i];
    
    computeKernel(kernel, size, radius);

    averageFilter(imgMean, buffer, image, kernel, size, radius);
    averageFilter(guideMean, buffer, guide, kernel, size, radius);
    averageFilter(guideCorr, buffer, sqGuide, kernel, size, radius);
    averageFilter(imgGuideCorr, buffer, imgGuide, kernel, size, radius);

    for (int i = 0; i < imageSize; i++)
        guideVar[i] = guideCorr[i] - guideMean[i] * guideMean[i];

    for (int i = 0; i < imageSize; i++)
        imgGuideCov[i] = imgGuideCorr[i] - imgMean[i] * guideMean[i];

    for (int i = 0; i < imageSize; i++)
        alpha[i] = imgGuideCov[i] / (guideVar[i] + epsilon);

    for (int i = 0; i < imageSize; i++)
        beta[i] = imgMean[i] - alpha[i] * guideMean[i];

    averageFilter(alphaMean, buffer, alpha, kernel, size, radius);
    averageFilter(betaMean, buffer, beta, kernel, size, radius);

    filtered.resize(imageSize);
    for (int i = 0; i < imageSize; i++)
        filtered[i] = alphaMean[i] * guide[i] + betaMean[i];
}
