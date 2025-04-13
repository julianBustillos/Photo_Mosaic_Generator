#include "ColorUtils.h"
#include <numbers>


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
}


void ColorUtils::BGRtoHSV(double& hue, double& saturation, double& value, uchar blue, uchar green, uchar red)
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

void ColorUtils::HSVtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double value)
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

void ColorUtils::BGRtoHSL(double& hue, double& saturation, double& lightness, uchar blue, uchar green, uchar red)
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

void ColorUtils::HSLtoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double lightness)
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

void ColorUtils::BGRtoHSI(double& hue, double& saturation, double& intensity, uchar blue, uchar green, uchar red)
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

void ColorUtils::HSItoBGR(uchar& blue, uchar& green, uchar& red, double hue, double saturation, double intensity)
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

void ColorUtils::BGRtoLUV(double& L, double& u, double& v, uchar blue, uchar green, uchar red)
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

void ColorUtils::LUVtoBGR(uchar& blue, uchar& green, uchar& red, double L, double u, double v)
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

void ColorUtils::BGRtoLAB(double& L, double& a, double& b, uchar blue, uchar green, uchar red)
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

void ColorUtils::LABtoBGR(uchar& blue, uchar& green, uchar& red, double L, double a, double b)
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



