// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include "clipcolorstock.h"
#include "perceptualcolormatch.h"
#include <limits>

namespace Unbound {
// Candidate fallback policy: nearest stock CIEDE2000 color to the
// spatially averaged gradient, with opacity composited over neutral clip grey.
// Texture is a display detail and does not change the fallback. Stock swatches
// and their native enum order are preserved; this policy needs live visual QA.
struct ClipColorFallback {
    static double linear(double c) {
        return c <= .04045 ? c / 12.92 : std::pow((c + .055) / 1.055, 2.4);
    }
    static QString nearest(const ColorPreset &paint) {
        double average[3]{};
        // Fixed integration is command-time only. Interpolate in the same RGB
        // space as the paint gradient, then compare averaged linear-light RGB.
        constexpr int samples = 64;
        for (int i = 0; i < samples; ++i) {
            const double p = (i + .5) / samples;
            QColor color = paint.color;
            double alpha = paint.opacity;
            if (!paint.stops.isEmpty()) {
                auto a = paint.stops.front(), b = paint.stops.back();
                for (const auto &stop : paint.stops) {
                    if (stop.position <= p) a = stop;
                    if (stop.position >= p) { b = stop; break; }
                }
                const double t = b.position > a.position
                    ? qBound(0., (p - a.position) / (b.position - a.position), 1.) : 0.;
                color = QColor::fromRgbF(a.color.redF() * (1-t) + b.color.redF() * t,
                                         a.color.greenF() * (1-t) + b.color.greenF() * t,
                                         a.color.blueF() * (1-t) + b.color.blueF() * t);
                alpha *= a.opacity * (1-t) + b.opacity * t;
            }
            const double channels[]{color.redF(), color.greenF(), color.blueF()};
            for (int c = 0; c < 3; ++c)
                average[c] += (linear(channels[c]) * alpha + linear(.25) * (1-alpha)) / samples;
        }
        const auto gamma = [](double v) { return v <= .0031308 ? 12.92*v : 1.055*std::pow(v, 1/2.4)-.055; };
        const auto representative = QColor::fromRgbF(gamma(average[0]), gamma(average[1]), gamma(average[2]));
        QString best;
        double distance = std::numeric_limits<double>::max();
        for (const auto &candidate : clipColorStockReferences()) {
            const auto next = colorDifference2000(representative, QColor(candidate.second));
            if (next < distance) { best = candidate.first; distance = next; }
        }
        return best;
    }
};
} // namespace Unbound
