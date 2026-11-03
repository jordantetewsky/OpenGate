// SPDX-License-Identifier: MIT
#pragma once
#include "colorpaintreadability.h"
#include "colorpainttexture.h"
#include <QLinearGradient>
namespace Unbound {
// Compile/draw preset paint and its label safety policy. No scene, input,
// assignment, storage or command mutation occurs during drawing.
class ColorPresetPaint {
  public:
    QBrush brush(const ColorPreset &style, QRectF bounds) const {
        const auto radians = (style.angle - 90) * 3.14159265358979323846 / 180.;
        const QPointF vector(std::cos(radians) * bounds.width(),
                             std::sin(radians) * bounds.height());
        QLinearGradient g(bounds.center() - vector / 2, bounds.center() + vector / 2);
        if (style.stops.isEmpty()) {
            auto c = style.color;
            c.setAlphaF(style.opacity);
            g.setColorAt(0, c);
            g.setColorAt(1, c);
        } else
            for (const auto &s : style.stops) {
                auto c = s.color;
                c.setAlphaF(style.opacity * s.opacity);
                g.setColorAt(s.position, c);
            }
        return QBrush(g);
    }
    void fill(QPainter &p, QRectF rect, const ColorPreset &style, QRectF basis = {},
              bool textures = true) {
        if (basis.isEmpty())
            basis = rect;
        p.fillRect(rect, brush(style, basis));
        if (textures) {
            const auto image = tiles.tile(style.texture, style.strength);
            if (!image.isNull()) {
                p.save();
                p.setBrushOrigin(basis.topLeft());
                p.fillRect(rect, QBrush(image));
                p.restore();
            }
        }
    }
    ColorLabelStyle label(const ColorPreset &style, QColor under = QColor(40, 40, 46),
                          bool textures = true) const {
        if (!style.stops.isEmpty() || (textures && style.texture != "flat" && style.strength > 0))
            return colorLabelStyle({}, true);
        QVector<QColor> samples;
        const auto over = [&](QColor c, double alpha) {
            return QColor::fromRgbF(c.redF() * alpha + under.redF() * (1 - alpha),
                                    c.greenF() * alpha + under.greenF() * (1 - alpha),
                                    c.blueF() * alpha + under.blueF() * (1 - alpha));
        };
        samples.append(over(style.color, style.opacity));
        return colorLabelStyle(samples, textures && style.texture != "flat" && style.strength > 0);
    }

  private:
    ColorPaintTexture tiles;
};
} // namespace Unbound
