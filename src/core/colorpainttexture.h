// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include <QCache>
#include <QImage>
#include <QPainter>
namespace Unbound {
// Bounded deterministic static texture tiles; no animation or preset ownership.
class ColorPaintTexture {
  public:
    QImage tile(const QString &kind, double strength) {
        if (kind == "flat" || strength <= 0)
            return {};
        const auto amount = qBound(0, qRound(strength * 100), 100);
        const auto key = kind + ":" + QString::number(amount);
        if (auto image = cache.object(key))
            return *image;
        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        quint32 seed = 173;
        const auto random = [&]() {
            seed = seed * 1664525u + 1013904223u;
            return double(seed) / 4294967296.;
        };
        const double alpha = amount / 100.;
        auto ink = [&](bool light, double opacity) {
            QColor c = light ? Qt::white : Qt::black;
            c.setAlphaF(qBound(0., opacity * alpha, 1.));
            return c;
        };
        if (kind.startsWith("grain")) {
            const int size = kind == "grain-coarse" ? 4 : kind == "grain-medium" ? 2 : 1;
            for (int y = 0; y < 64; y += size)
                for (int x = 0; x < 64; x += size) {
                    const bool light = random() > .5;
                    p.fillRect(x, y, size, size, ink(light, .08 + random() * .34));
                }
        } else if (kind.startsWith("crosshatch")) {
            const int step = kind == "crosshatch-fine" ? 4 : kind == "crosshatch-coarse" ? 16 : 8;
            const double width = step == 4 ? .65 : step == 16 ? 1.7 : 1.;
            p.setRenderHint(QPainter::Antialiasing);
            for (int layer = 0; layer < 2; ++layer) {
                p.setPen(QPen(ink(layer, layer ? .22 : .36), width));
                const double d = layer * width;
                for (int x = -64; x <= 128; x += step) {
                    p.drawLine(QPointF(x + d, 0), QPointF(x + 64 + d, 64));
                    p.drawLine(QPointF(x + d, 0), QPointF(x - 64 + d, 64));
                }
            }
        } else if (kind.startsWith("stipple")) {
            const int step = kind == "stipple-fine" ? 4 : kind == "stipple-coarse" ? 16 : 8;
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            for (int y = 0; y < 64; y += step)
                for (int x = 0; x < 64; x += step) {
                    const auto dx = x + step * .25 + random() * step * .375,
                               dy = y + step * .25 + random() * step * .375,
                               r = (step == 4    ? .4
                                    : step == 16 ? 1.5
                                                 : .7) +
                                   random() * (step == 4    ? .35
                                               : step == 16 ? 1.
                                                            : .6);
                    p.setBrush(ink(false, .5));
                    p.drawEllipse(QPointF(dx, dy), r, r);
                    p.setBrush(ink(true, .28));
                    p.drawEllipse(QPointF(dx + r, dy + r), r * .7, r * .7);
                }
        } else if (kind.startsWith("brushed")) {
            const bool fine = kind == "brushed-fine", coarse = kind == "brushed-coarse";
            const int step = coarse ? 4 : 1;
            const double height = fine ? .5 : coarse ? 3 : 1;
            for (int y = 0; y < 64; y += step) {
                const bool light = random() > .5;
                p.fillRect(QRectF(0, y, 64, height), ink(light, .04 + random() * .24));
                for (int j = 0; j < 3; ++j) {
                    const auto color = ink(true, random() * .15);
                    const int x = int(random() * 64), width = (fine     ? 2
                                                               : coarse ? 8
                                                                        : 4) +
                                                              int(random() * (fine     ? 9
                                                                              : coarse ? 28
                                                                                       : 18));
                    p.fillRect(QRectF(x, y, width, height), color);
                }
            }
        }
        p.end();
        cache.insert(key, new QImage(image), 1);
        return image;
    }

  private:
    QCache<QString, QImage> cache{32};
};
} // namespace Unbound
