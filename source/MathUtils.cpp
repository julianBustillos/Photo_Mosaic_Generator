#include "MathUtils.h"
#include <numbers>


/*
    Unnamed namespace methods called by MathUtils methods
*/

namespace
{
    double cubeRoot(double t)
    {
        double inv3 = 1. / 3.;
        double root = 1.4774329094 - 0.8414323527 / (t + 0.7387320679);

        while (abs(root * root * root - t) > 0.000001)
        {
            root = (2. * root + t / (root * root)) * inv3;
        }

        return root;
    }

    class SamplingFilter
    {
    public:
        SamplingFilter(double support) : _support(support) {};
        void setScale(double scale) { _scale = scale > 1. ? scale : 1.; _invScale = 1. / _scale; };
        double getSupport() const { return _support * _scale; };
        virtual double compute(double xPos, double center) const = 0;

    protected:
        const double _support;
        double _scale;
        double _invScale;
    };

    double sinc(double x)
    {
        if (x == 0.0)
        {
            return 1.0;
        }
        x = x * std::numbers::pi;
        return sin(x) / x;
    }

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
            constexpr double lanczos_a_param = 3.0;

            double x = (xPos - center) * _invScale;
            if (-lanczos_a_param <= x && x < lanczos_a_param)
            {
                return sinc(x) * sinc(x / lanczos_a_param);
            }
            return 0.0;
        }
    };

    void computeCoefficients(int inSize, int outSize, int min, int max, int& nbCoeffs, std::vector<double>& coeffs, std::vector<int>& bounds, SamplingFilter* filter)
    {
        const double scale = (double)(max - min) / (double)outSize;
        filter->setScale(scale);
        const double support = filter->getSupport();
        nbCoeffs = (int)std::ceil(support) * 2 + 1;

        coeffs.resize(outSize * nbCoeffs);
        bounds.resize(outSize * 2);

        constexpr double halfPixel = 0.5;
        for (int xOut = 0; xOut < outSize; xOut++)
        {
            double center = min + (xOut + halfPixel) * scale;
            
            int xMin = (int)(center - support + halfPixel);
            if (xMin < 0)
                xMin = 0;

            int xMax = (int)(center + support + halfPixel);
            if (xMax > inSize)
                xMax = inSize;

            double* coeff = &coeffs[xOut * nbCoeffs];

            int xSize = xMax - xMin;
            double sum = 0.;
            for (int x = 0; x < xSize; x++)
            {
                coeff[x] = filter->compute(x + xMin + halfPixel, center);
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

    void resampleHorizontal(cv::Mat& output, const cv::Mat& input, int offset, int nbCoeffs, const std::vector<double>& coeffs, const std::vector<int> bounds)
    {
        constexpr unsigned int PrecisionBits = 32 - 8 - 2;
        constexpr double PrecisionShift = (double)(1U << PrecisionBits);
        constexpr double PixelInit = 1U << (PrecisionBits - 1U);

        std::vector<double> shiftedCoeffs;
        shiftedCoeffs.reserve(coeffs.size());

        for (double c : coeffs)
        {
            shiftedCoeffs.emplace_back(std::round(c * PrecisionShift));
        }

        for (int yOut = 0; yOut < output.size().height; yOut++)
        {
            for (int xOut = 0; xOut < output.size().width; xOut++)
            {
                const int xMin = bounds[xOut * 2 + 0];
                const int xSize = bounds[xOut * 2 + 1];
                const double* coeff = &shiftedCoeffs[xOut * nbCoeffs];
                for (int c = 0; c < input.channels(); c++)
                {
                    double pixel = PixelInit;
                    for (int x = 0; x < xSize; x++)
                    {
                        pixel += (double)(input.ptr<uchar>(yOut + offset, x + xMin)[c]) * coeff[x];
                    }
                    output.ptr<uchar>(yOut, xOut)[c] = (uchar)MathUtils::clip<int>((int)pixel >> PrecisionBits, 0, 255);
                }
            }
        }
    }

    void resampleVertical(cv::Mat& output, const cv::Mat& input, int nbCoeffs, const std::vector<double>& coeffs, const std::vector<int> bounds)
    {
        output = output.t();
        resampleHorizontal(output, input.t(), 0, nbCoeffs, coeffs, bounds);
        output = output.t();
    }

    void resample(cv::Mat& target, const cv::Size& targetSize, const cv::Mat& source, const cv::Rect& box, SamplingFilter* filter)
    {
        cv::Mat temp;
        const bool doHoriSampling = targetSize.width != box.width;
        const bool doVertSampling = targetSize.height != box.height;

        cv::Vec4i limits(box.x, box.y, box.x + box.width, box.y + box.height);

        int nbCoeffsHori = 0, nbCoeffsVert = 0;
        std::vector<double> coeffsHori, coeffsVert;
        std::vector<int> boundsHori, boundsVert;

        if (doHoriSampling)
            computeCoefficients(source.size().width, targetSize.width, limits[0], limits[2], nbCoeffsHori, coeffsHori, boundsHori, filter);

        if (doVertSampling)
            computeCoefficients(source.size().height, targetSize.height, limits[1], limits[3], nbCoeffsVert, coeffsVert, boundsVert, filter);

        if (doHoriSampling)
        {
            const int yLowBound = boundsVert[0];
            const int yHighBound = boundsVert[targetSize.height * 2 - 2] + boundsVert[targetSize.height * 2 - 1];

            for (int i = 0; i < targetSize.height; ++i)
                boundsVert[i * 2] -= yLowBound;

            cv::Mat& output = doVertSampling ? temp : target;
            output.create(yHighBound - yLowBound, targetSize.width, source.type());
            resampleHorizontal(output, source, yLowBound, nbCoeffsHori, coeffsHori, boundsHori);
        }

        if (doVertSampling)
        {
            const cv::Mat& input = doHoriSampling ? temp : source;
            target.create(targetSize, source.type());
            resampleVertical(target, input, nbCoeffsVert, coeffsVert, boundsVert);
        }

        if (!doHoriSampling && !doVertSampling)
        {
            target.create(targetSize, source.type());
            //TODO no sampling MODE = copy box!!!
        }
    }

    void getGaussianApproxBoxRadiuses(double sigma, int *boxRadius)
    {
        int n = MathUtils::BlurNbBoxes;
        double wIdeal = sqrt(12. * sigma * sigma / n + 1.);
        int wl = (int)floor(wIdeal);
        if ((wl % 2) == 0)
            wl--;
        int wu = wl + 2;
        int m = (int)round((12. * sigma * sigma - n * wl * wl - 4. * n * wl - 3 * n) / (-4. * wl - 4.));
        int wlr = (wl - 1) / 2;
        int wur = (wu - 1) / 2;

        for (int k = 0; k < n; k++)
            boxRadius[k] = (k < m)?wlr:wur;
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
}


/*
    MathUtils methods implementation 
*/

void MathUtils::computeGrayscale(cv::Mat& target, const cv::Mat& source)
{
    target.create(source.size(), CV_8UC1);
    for (int i = 0; i < source.size().height; i++)
    {
        for (int j = 0; j < source.size().width; j++)
        {
            target.ptr(i, j)[0] = (uchar)std::round(source.ptr(i, j)[0] * 0.114 + source.ptr(i, j)[1] * 0.587 + source.ptr(i, j)[2] * 0.299);
        }
    }
}

void MathUtils::computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, Filter filter)
{
    cv::Rect box(0, 0, source.size().width, source.size().height);
    computeImageResampling(target, targetSize, source, box, filter);
}

void MathUtils::computeImageResampling(cv::Mat& target, const cv::Size targetSize, const cv::Mat& source, const cv::Rect& box, Filter filter)
{
    SamplingFilter* samplingFilter = nullptr;

    if (filter == MathUtils::AREA)
    {
        samplingFilter = new AreaFilter();
    }
    else if (filter == MathUtils::BICUBIC)
    {
        samplingFilter = new BicubicFilter();
    }
    else if (filter == MathUtils::LANCZOS)
    {
        samplingFilter = new LanczosFilter();
    }

    if (filter)
    {
        resample(target, targetSize, source, box, samplingFilter);
        delete samplingFilter;
        samplingFilter = nullptr;
    }
}

void MathUtils::computeImageDHash(const cv::Mat& image, Hash& hash)
{
    int hashId = 0;
    cv::Mat grayscaleImage, hashImage;
    cv::Size dhashHoriSize(HashSize + 1, HashSize);
    cv::Size dhashVertSize(HashSize, HashSize + 1);

    computeGrayscale(grayscaleImage, image);

    computeImageResampling(hashImage, dhashHoriSize, grayscaleImage, LANCZOS);
    for (int i = 0; i < dhashHoriSize.height; i++)
    {
        for (int j = 0; j < dhashHoriSize.width - 1; j++, hashId++)
        {
            if (hashImage.ptr(i, j)[0] < hashImage.ptr(i, j + 1)[0])
            {
                hash.set(hashId);
            }
        }
    }

    computeImageResampling(hashImage, dhashVertSize, grayscaleImage, LANCZOS);
    for (int i = 0; i < dhashVertSize.height - 1; i++)
    {
        for (int j = 0; j < dhashVertSize.width; j++, hashId++)
        {
            if (hashImage.ptr(i, j)[0] < hashImage.ptr(i + 1, j)[0])
            {
                hash.set(hashId);
            }
        }
    }
}

void MathUtils::applyGaussianBlur(uchar* image, const cv::Size& size, double sigma)
{
    uchar* buffer = new uchar[3 * size.width * size.height];
    if (!buffer)
        return;

    int boxRadius[BlurNbBoxes];
    getGaussianApproxBoxRadiuses(sigma, boxRadius);

    if (boxRadius[BlurNbBoxes - 1] <= std::min(size.width, size.height) / 2)
    {
        for (int k = 0; k < BlurNbBoxes; k++)
            applyBoxBlur(image, buffer, size, boxRadius[k]);
    }

    delete[] buffer;
}

void MathUtils::computeImageBGRFeatures(const cv::Mat& image, const cv::Rect& box, double* features, int featureDirSubdivision)
{
    int blockWidth = (int)ceil(box.width / (double)featureDirSubdivision);
    int blockHeight = (int)ceil(box.height / (double)featureDirSubdivision);

    for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++)
        features[k] = 0;

    for (int i = 0; i < box.height; i++)
    {
        for (int j = 0; j < box.width; j++)
        {
            int blockId = featureDirSubdivision * (i / blockHeight) + j / blockWidth;
            for (int c = 0; c < 3; c++)
                features[3 * blockId + c] += image.ptr(box.y + i, box.x + j)[c];
        }
    }

    for (int k = 0; k < 3 * featureDirSubdivision * featureDirSubdivision; k++)
    {
        int corrBlockHeight = (k < featureDirSubdivision * (featureDirSubdivision - 1) * 3) ? blockHeight : box.height - (featureDirSubdivision - 1) * blockHeight;
        int corrBlockWidth = (((k / 4 + 1) % featureDirSubdivision) != 0) ? blockWidth : box.width - (featureDirSubdivision - 1) * blockWidth;
        features[k] /= corrBlockWidth * corrBlockHeight;
    }
}

double MathUtils::BGRFeatureDistance(const double* vec1, const double* vec2, int size)
{
    //Use deltaE distance
    double sumDist = 0.;
    for (int i = 0; i < 3 * size; i += 3)
    {
        double dB = vec1[i] - vec2[i];
        double dG = vec1[i + 1] - vec2[i + 1];
        double dR = vec1[i + 2] - vec2[i + 2];
        double mR = (vec1[i + 2] + vec2[i + 2]) / 2.;
        double sqDist = (2. + mR / 256.) * dR * dR + 4 * dG * dG + (2. + (255. - mR) / 256.) * dB * dB;
        sumDist += sqrt(sqDist);
    }

    return sumDist;
}

void MathUtils::convertBGRtoHSV(double& hue, double& saturation, double& value, uchar blue, uchar green, uchar red)
{
    double B = blue / 255.;
    double G = green / 255.;
    double R = red / 255.;

    double min = B, max = B;
    min = std::min(min, G);
    min = std::min(min, R);
    max = std::max(max, G);
    max = std::max(max, R);

    if (min == max)
        hue = 0.;
    else if (max == B)
        hue = std::numbers::pi / 3. * (4 + (R - G) / (max - min));
    else if (max == G)
        hue = std::numbers::pi / 3. * (2 + (B - R) / (max - min));
    else
        hue = std::numbers::pi / 3. * (G - B) / (max - min);

    if (hue < 0.)
        hue += 2 * std::numbers::pi;

    if (max == 0.)
        saturation = 0.;
    else
        saturation = (max - min) / max;

    value = max;

    hue = clip<double>(hue, 0., 2 * std::numbers::pi);
    saturation = clip<double>(saturation, 0., 1.);
    value = clip<double>(value, 0., 1.);
}

void MathUtils::convertHSVtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double value)
{
    double C = value * saturation;
    double H = hue * 3. / std::numbers::pi;
    double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

    double B, G, R;
    if (H < 0. || 6. < H)
    {
        B = 0;
        G = 0;
        R = 0;
    }
    else if (H <= 1.)
    {
        B = 0.;
        G = X;
        R = C;
    }
    else if (H <= 2.)
    {
        B = 0.;
        G = C;
        R = X;
    }
    else if (H <= 3.)
    {
        B = X;
        G = C;
        R = 0.;
    }
    else if (H <= 4.)
    {
        B = C;
        G = X;
        R = 0.;
    }
    else if (H <= 5.)
    {
        B = C;
        G = 0.;
        R = X;
    }
    else if (H <= 6.)
    {
        B = X;
        G = 0.;
        R = C;
    }

    double m = value - C;

    blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
    red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
}

void MathUtils::convertBGRtoHSL(double& hue, double& saturation, double& lightness, uchar blue, uchar green, uchar red)
{
    double B = blue / 255.;
    double G = green / 255.;
    double R = red / 255.;

    double min = B, max = B;
    min = std::min(min, G);
    min = std::min(min, R);
    max = std::max(max, G);
    max = std::max(max, R);

    if (min == max)
        hue = 0.;
    else if (max == B)
        hue = std::numbers::pi / 3. * (4 + (R - G) / (max - min));
    else if (max == G)
        hue = std::numbers::pi / 3. * (2 + (B - R) / (max - min));
    else
        hue = std::numbers::pi / 3. * (G - B) / (max - min);

    if (hue < 0.)
        hue += 2 * std::numbers::pi;

    lightness = (max + min) / 2.;

    if (max == 0. || min == 1.)
        saturation = 0.;
    else
        saturation = (max - lightness) / (std::min(lightness, 1. - lightness));

    hue = clip<double>(hue, 0., 2 * std::numbers::pi);
    saturation = clip<double>(saturation, 0., 1.);
    lightness = clip<double>(lightness, 0., 1.);
}

void MathUtils::convertHSLtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double lightness)
{
    double C = (1. - std::abs(2. * lightness - 1.)) * saturation;
    double H = hue * 3. / std::numbers::pi;
    double X = C * (1. - std::abs(std::fmod(H, 2.) - 1.));

    double B, G, R;
    if (H < 0. || 6. < H)
    {
        B = 0;
        G = 0;
        R = 0;
    }
    else if (H <= 1.)
    {
        B = 0.;
        G = X;
        R = C;
    }
    else if (H <= 2.)
    {
        B = 0.;
        G = C;
        R = X;
    }
    else if (H <= 3.)
    {
        B = X;
        G = C;
        R = 0.;
    }
    else if (H <= 4.)
    {
        B = C;
        G = X;
        R = 0.;
    }
    else if (H <= 5.)
    {
        B = C;
        G = 0.;
        R = X;
    }
    else if (H <= 6.)
    {
        B = X;
        G = 0.;
        R = C;
    }

    double m = lightness - C / 2.;

    blue = (uchar)clip<int>((int)round((B + m) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((G + m) * 255.), 0, 255);
    red = (uchar)clip<int>((int)round((R + m) * 255.), 0, 255);
}

void MathUtils::convertBGRtoHSI(double& hue, double& saturation, double& intensity, uchar blue, uchar green, uchar red)
{
    double B = blue / 255.;
    double G = green / 255.;
    double R = red / 255.;
    double i = B + G + R;
    intensity = i / 3;

    if (R == G && G == B)
    {
        hue = 0.;
        saturation = 0.;
    }
    else
    {
        double w = 0.5 * (2 * R - G - B) / sqrt((R - G) * (R - G) + (R - B) * (G - B));
        w = clip<double>(w, -1., 1.);
        hue = acos(w);
        if (B > G)
            hue = 2 * std::numbers::pi - hue;
        double min;
        if (R <= B && R <= G)
            min = R;
        else if (B <= G)
            min = B;
        else
            min = G;
        saturation = 1 - 3 * min / i;
    }

    hue = clip<double>(hue, 0., 2 * std::numbers::pi);
    saturation = clip<double>(saturation, 0., 1.);
    intensity = clip<double>(intensity, 0., 1.);
}

void MathUtils::convertHSItoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double intensity)
{
    double B, G, R;

    if (saturation == 0.)
        B = G = R = intensity;
    else
    {
        if ((hue >= 0.) && (hue < 2. * std::numbers::pi / 3.))
        {
            B = (1. - saturation) / 3.;
            R = (1. + saturation * cos(hue) / cos(std::numbers::pi / 3. - hue)) / 3.;
            G = 1. - R - B;
        }
        else if ((hue >= 2. * std::numbers::pi / 3.) && (hue < 4. * std::numbers::pi / 3.))
        {
            hue = hue - 2. * std::numbers::pi / 3.;
            R = (1. - saturation) / 3.;
            G = (1. + saturation * cos(hue) / cos(std::numbers::pi / 3. - hue)) / 3.;
            B = 1. - R - G;
        }
        else if ((hue >= 4. * std::numbers::pi / 3.) && (hue < 2. * std::numbers::pi))
        {
            hue = hue - 4. * std::numbers::pi / 3.;
            G = (1. - saturation) / 3.;
            B = (1. + saturation * cos(hue) / cos(std::numbers::pi / 3. - hue)) / 3.;
            R = 1. - B - G;
        }
        else
        {
            B = G = R = 0.;
        }

        B *= 3 * intensity;
        G *= 3 * intensity;
        R *= 3 * intensity;
    }

    blue = (uchar)clip<int>((int)round(B * 255.), 0, 255);
    green = (uchar)clip<int>((int)round(G * 255.), 0, 255);
    red = (uchar)clip<int>((int)round(R * 255.), 0, 255);
}

void MathUtils::convertBGRtoLUV(double& L, double& u, double& v, uchar blue, uchar green, uchar red)
{
    //Using illuminant D65 as white reference

    if (blue == 0 && green == 0 && red == 0)
    {
        L = 0.;
        u = 0.;
        v = 0.;
        return;
    }

    double R = (double)red / 255.;
    double G = (double)green / 255.;
    double B = (double)blue / 255.;

    double X = (0.49 * R + 0.31 * G + 0.2 * B) / 0.17697;
    double Y = (0.17697 * R + 0.8124 * G + 0.01063 * B) / 0.17697;
    double Z = (0.01 * G + 0.99 * B) / 0.17697;

    double Y_norm = Y / 100.;
    double div = X + 15. * Y + 3. * Z;
    double u_p = 4. * X / div;
    double v_p = 9. * Y / div;

    if (Y_norm > 0.008856)
        L = 116. * cubeRoot(Y_norm) - 16.;
    else
        L = Y_norm * 903.296296;

    u = 13. * L * (u_p - 0.197840);
    v = 13. * L * (v_p - 0.468336);

    L = clip<double>(L, 0., 100.);
    u = clip<double>(u, -100., 100.);
    v = clip<double>(v, -100., 100.);
}

void MathUtils::convertLUVtoBGR(uchar& blue, uchar& green, uchar& red, double L, double u, double v)
{
    //Using illuminant D65 as white reference

    if (L == 0. && u == 0. && v == 0.)
    {
        blue = 0;
        green = 0;
        red = 0;
        return;
    }

    double u_p = u / (13. * L) + 0.197840;
    double v_p = v / (13. * L) + 0.468336;

    double Y;
    if (L > 8.)
        Y = 100. * (L + 16.) * (L + 16.) * (L + 16.) / 1560896.;
    else
        Y = 100. * L * 0.001107;

    double X = Y * 9. * u_p / (4. * v_p);
    double Z = Y * (12. - 3. * u_p - 20. * v_p) / (4. * v_p);

    red = (uchar)clip<int>((int)round((0.41847 * X - 0.15866 * Y - 0.082835 * Z) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((-0.091169 * X + 0.25243 * Y + 0.015708 * Z) * 255.), 0, 255);
    blue = (uchar)clip<int>((int)round((0.0009209 * X - 0.0025498 * Y + 0.1786 * Z) * 255.), 0, 255);
}

void MathUtils::convertBGRtoLAB(double& L, double& a, double& b, uchar blue, uchar green, uchar red)
{
    //Using illuminant D65 as white reference

    if (blue == 0 && green == 0 && red == 0)
    {
        L = 0.;
        a = 0.;
        b = 0.;
        return;
    }

    double R = (double)red / 255.;
    double G = (double)green / 255.;
    double B = (double)blue / 255.;

    double X_norm = (0.49 * R + 0.31 * G + 0.2 * B) / 16.820804;
    double Y_norm = (0.17697 * R + 0.8124 * G + 0.01063 * B) / 17.697;
    double Z_norm = (0.01 * G + 0.99 * B) / 19.269201;

    double X_var, Y_var, Z_var;

    if (X_norm > 0.008856)
        X_var = cubeRoot(X_norm);
    else
        X_var = X_norm * 7.787037 + 0.137931;

    if (Y_norm > 0.008856)
        Y_var = cubeRoot(Y_norm);
    else
        Y_var = Y_norm * 7.787037 + 0.137931;

    if (Z_norm > 0.008856)
        Z_var = cubeRoot(Z_norm);
    else
        Z_var = Z_norm * 7.787037 + 0.137931;

    L = 116. * Y_var - 16.;
    a = 500 * (X_var - Y_var);
    b = 200 * (Y_var - Z_var);

    L = clip<double>(L, 0., 100.);
    a = clip<double>(a, -100., 100.);
    b = clip<double>(b, -100., 100.);
}

void MathUtils::convertLABtoBGR(uchar& blue, uchar& green, uchar& red, double L, double a, double b)
{
    //Using illuminant D65 as white reference

    if (L == 0. && a == 0. && b == 0.)
    {
        blue = 0;
        green = 0;
        red = 0;
        return;
    }

    double Y_var = (L + 16.) / 116.;
    double X_var = Y_var + a / 500.;
    double Z_var = Y_var - b / 200.;

    double X, Y, Z;

    if (X_var > 0.206896)
        X = 95.0489 * X_var * X_var * X_var;
    else
        X = 12.206042 * X_var - 1.683594;

    if (Y_var > 0.206896)
        Y = 100. * Y_var * Y_var * Y_var;
    else
        Y = 12.841855 * Y_var - 1.771290;

    if (Z_var > 0.206896)
        Z = 108.884 * Z_var * Z_var * Z_var;
    else
        Z = 13.982725 * Z_var - 1.928652;

    red = (uchar)clip<int>((int)round((0.41847 * X - 0.15866 * Y - 0.082835 * Z) * 255.), 0, 255);
    green = (uchar)clip<int>((int)round((-0.091169 * X + 0.25243 * Y + 0.015708 * Z) * 255.), 0, 255);
    blue = (uchar)clip<int>((int)round((0.0009209 * X - 0.0025498 * Y + 0.1786 * Z) * 255.), 0, 255);
}



