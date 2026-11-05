// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include <QColor>
#include <QRectF>
#include <QSharedPointer>
#include <QVector>
namespace Unbound {
// Match large solid body primitives to unambiguous clip bounds in viewport space.
struct ClipFillRegion {
    QRectF rect;
    QColor color;
    QRectF visible{};
    QSharedPointer<const ColorPreset> style{};
    QVector<QRectF> labels{};
    quintptr graphicsIdentity = 0; // Ephemeral QGraphicsObject address, never a project ID.
};
inline bool clipBodyPrimitive(const QRectF &rect, QColor source) {
    return !rect.isEmpty() && source.alpha() > 0 && source.hsvSaturationF() > .25 &&
           source.valueF() > .2;
}
inline bool clipBodyContains(const ClipFillRegion &region, const QRectF &rect) {
    const auto visible = region.visible.isEmpty() ? region.rect : region.visible;
    return !region.rect.isEmpty() && region.rect.adjusted(-1, -1, 1, 1).contains(rect) &&
           rect.width() * rect.height() >= visible.width() * visible.height() * .35;
}
inline QColor clipRegionReplacement(const QRectF &rect, QColor source,
                                    const QVector<ClipFillRegion> &regions) {
    if (!clipBodyPrimitive(rect, source))
        return {};
    QColor result;
    for (const auto &region : regions) {
        if (!clipBodyContains(region, rect))
            continue;
        if (result.isValid() && result != region.color)
            return {}; // Ambiguous overlap stays native.
        result = region.color;
    }
    return result;
}
} // namespace Unbound
