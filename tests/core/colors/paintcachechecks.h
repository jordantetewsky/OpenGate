// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colorpresetpaint.h"
namespace OpenGateTest {
template <class Check> void paintCacheChecks(Check check) {
    Unbound::ColorPresetPaint paint;
    for (double angle : {0., 90., 135., 210., 315., 360.}) {
        Unbound::ColorPreset preset;
        preset.angle = angle;
        preset.color = Qt::red;
        preset.stops = {{"a", Qt::red, 0, 1},
                        {"b", Qt::white, .2, 1},
                        {"c", Qt::green, .49, 1},
                        {"d", Qt::black, .7, 1},
                        {"e", Qt::blue, 1, 1}};
        const auto definition = QSharedPointer<const Unbound::ColorPreset>::create(preset);
        for (const QRectF bounds :
             {QRectF(4, 6, 320, 22), QRectF(35, 27, 31, 200), QRectF(13, 15, 170, 100)}) {
            QImage reference(380, 250, QImage::Format_ARGB32_Premultiplied),
                cached(reference.size(), reference.format());
            reference.fill(Qt::transparent);
            cached.fill(Qt::transparent);
            {
                QPainter p(&reference);
                paint.fill(p, bounds, preset, {}, false);
            }
            {
                QPainter p(&cached);
                paint.fill(p, bounds, definition, {}, false);
            }
            int maximum = 0;
            for (int y = bounds.top(); y < bounds.bottom(); ++y)
                for (int x = bounds.left(); x < bounds.right(); ++x) {
                    const auto a = reference.pixelColor(x, y), b = cached.pixelColor(x, y);
                    maximum =
                        qMax(maximum,
                             qMax(qMax(qAbs(a.red() - b.red()), qAbs(a.green() - b.green())),
                                  qMax(qAbs(a.blue() - b.blue()), qAbs(a.alpha() - b.alpha()))));
                }
            check(maximum <= 2, "compiled immutable gradient preserves preview colors for "
                                "horizontal, vertical and diagonal resized clips");
        }
    }
}
} // namespace OpenGateTest
