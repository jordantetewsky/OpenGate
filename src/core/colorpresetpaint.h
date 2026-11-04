// SPDX-License-Identifier: MIT
#pragma once
#include "colorpaintreadability.h"
#include "colorpainttexture.h"
#include <QLinearGradient>
#include <QSharedPointer>
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
    QBrush brush(const QSharedPointer<const ColorPreset> &style, QRectF bounds,
                 QTransform basisToPainter = {}) {
        if (!style)
            return {};
        const auto key = reinterpret_cast<quintptr>(style.data());
        auto compiled = brushes.object(key);
        if (!compiled) {
            auto unit = *style;
            unit.angle = 90;
            const auto radians = (style->angle - 90) * 3.14159265358979323846 / 180.;
            compiled = new Compiled{style, brush(unit, QRectF(0, 0, 1, 1)), std::cos(radians),
                                    std::sin(radians)};
            brushes.insert(key, compiled);
        }
        auto result = compiled->normalized;
        const QPointF vector(compiled->x * bounds.width(), compiled->y * bounds.height());
        const auto start = bounds.center() - vector / 2;
        // Uniform scale/rotation preserves the preview's gradient projection,
        // including diagonal gradients on wide, shallow clip footers.
        const QTransform placement(vector.x(), vector.y(), -vector.y(), vector.x(), start.x(),
                                   start.y());
        result.setTransform(placement * basisToPainter);
        return result;
    }
    void fill(QPainter &p, QRectF rect, const ColorPreset &style, QRectF basis = {},
              bool textures = true, QTransform basisToPainter = {}) {
        if (basis.isEmpty())
            basis = rect;
        auto background = brush(style, basis);
        background.setTransform(basisToPainter);
        fillBrush(p, rect, style, basis, textures, basisToPainter, background);
    }
    void fill(QPainter &p, QRectF rect, const QSharedPointer<const ColorPreset> &style,
              QRectF basis = {}, bool textures = true, QTransform basisToPainter = {}) {
        if (!style)
            return;
        if (basis.isEmpty())
            basis = rect;
        fillBrush(p, rect, *style, basis, textures, basisToPainter,
                  brush(style, basis, basisToPainter));
    }
    void texture(QPainter &p, QRectF rect, const ColorPreset &style, QRectF basis,
                 QTransform basisToPainter = {}) {
        const auto image = tiles.tile(style.texture, style.strength);
        if (!image.isNull()) {
            p.save();
            p.setBrushOrigin(QPointF());
            p.setOpacity(p.opacity() * style.opacity);
            QBrush pattern(image);
            pattern.setTransform(QTransform::fromTranslate(basis.left(), basis.top()) *
                                 basisToPainter);
            p.fillRect(rect, pattern);
            p.restore();
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
    struct Compiled {
        QSharedPointer<const ColorPreset> definition;
        QBrush normalized;
        double x = 1, y = 0;
    };
    // Retaining the immutable definition prevents address reuse from colliding
    // with cached paint. Clip position/size is a transform, not another gradient.
    QCache<quintptr, Compiled> brushes{128};
    ColorPaintTexture tiles;
    void fillBrush(QPainter &p, QRectF rect, const ColorPreset &style, QRectF basis, bool textures,
                   QTransform basisToPainter, const QBrush &background) {
        p.save();
        p.setBrushOrigin(QPointF());
        p.fillRect(rect, background);
        p.restore();
        if (textures)
            texture(p, rect, style, basis, basisToPainter);
    }
};
} // namespace Unbound
