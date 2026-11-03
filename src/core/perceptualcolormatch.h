// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <algorithm>
#include <cmath>
#include <limits>
namespace Unbound {
// Color-distance mathematics only. Palette authority, gradient reduction,
// project writes and assignment history are explicitly outside this module.
struct ColorLab {
    double l, a, b;
};
inline ColorLab colorLab(QColor color) {
    const auto linear = [](double v) {
        return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
    };
    const auto r = linear(color.redF()), g = linear(color.greenF()), b = linear(color.blueF());
    const auto f = [](double v) {
        return v > 216. / 24389 ? std::cbrt(v) : v * (24389. / 27) / 116 + 16. / 116;
    };
    const auto x = f((r * .4124564 + g * .3575761 + b * .1804375) / .95047),
               y = f(r * .2126729 + g * .7151522 + b * .0721750),
               z = f((r * .0193339 + g * .1191920 + b * .9503041) / 1.08883);
    return {116 * y - 16, 500 * (x - y), 200 * (y - z)};
}
// Independent implementation of the published CIEDE2000 equations (kL=kC=kH=1).
// Sharma/Wu/Dalal 2005: https://doi.org/10.1002/col.20070
// Reference notes/data: https://hajim.rochester.edu/ece/sites/gsharma/ciede2000/
inline double colorDifference2000(ColorLab first, ColorLab second) {
    constexpr double pi = 3.14159265358979323846;
    const auto rad = [&](double d) { return d * pi / 180.; };
    const auto square = [](double x) { return x * x; };
    const auto seventh = [](double x) { return std::pow(x, 7); };
    const auto c1 = std::hypot(first.a, first.b), c2 = std::hypot(second.a, second.b),
               mean = (c1 + c2) / 2;
    const auto correction = .5 * (1 - std::sqrt(seventh(mean) / (seventh(mean) + seventh(25))));
    const auto a1 = first.a * (1 + correction), a2 = second.a * (1 + correction),
               cp1 = std::hypot(a1, first.b), cp2 = std::hypot(a2, second.b);
    const auto hue = [&](double a, double b) {
        if (a == 0 && b == 0)
            return 0.;
        const auto value = std::atan2(b, a) * 180 / pi;
        return value < 0 ? value + 360 : value;
    };
    const auto h1 = hue(a1, first.b), h2 = hue(a2, second.b);
    auto dh = h2 - h1;
    if (cp1 * cp2 == 0)
        dh = 0;
    else if (dh > 180)
        dh -= 360;
    else if (dh < -180)
        dh += 360;
    const auto dl = second.l - first.l, dc = cp2 - cp1,
               dH = 2 * std::sqrt(cp1 * cp2) * std::sin(rad(dh / 2)), L = (first.l + second.l) / 2,
               C = (cp1 + cp2) / 2;
    double H;
    if (cp1 * cp2 == 0)
        H = h1 + h2;
    else if (std::abs(h1 - h2) <= 180)
        H = (h1 + h2) / 2;
    else
        H = (h1 + h2 + (h1 + h2 < 360 ? 360 : -360)) / 2;
    const auto T = 1 - .17 * std::cos(rad(H - 30)) + .24 * std::cos(rad(2 * H)) +
                   .32 * std::cos(rad(3 * H + 6)) - .20 * std::cos(rad(4 * H - 63));
    const auto sl = 1 + .015 * square(L - 50) / std::sqrt(20 + square(L - 50)), sc = 1 + .045 * C,
               sh = 1 + .015 * C * T;
    const auto rotation = -2 * std::sqrt(seventh(C) / (seventh(C) + seventh(25))) *
                          std::sin(rad(60 * std::exp(-square((H - 275) / 25))));
    const auto l = dl / sl, c = dc / sc, h = dH / sh;
    return std::sqrt(std::max(0., l * l + c * c + h * h + rotation * c * h));
}
inline double colorDifference2000(QColor first, QColor second) {
    if (!first.isValid() || !second.isValid())
        return std::numeric_limits<double>::infinity();
    return colorDifference2000(colorLab(first), colorLab(second));
}
} // namespace Unbound
