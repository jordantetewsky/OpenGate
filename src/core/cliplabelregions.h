// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsObject>
#include <QVector>
#include <typeinfo>
namespace Unbound {
// Discover label geometry from native semantic item types, never from arbitrary
// image pixels. Returned rectangles contain no retained child-item pointers.
inline QVector<QRectF> clipLabelRegions(QGraphicsItem *clip) {
    QVector<QRectF> result;
    if (!clip)
        return result;
    const auto bounds = clip->sceneBoundingRect();
    auto pending = clip->childItems();
    int visited = 0;
    while (!pending.isEmpty() && visited++ < 256) {
        auto item = pending.takeLast();
        if (!item->isVisible())
            continue;
        const auto object = item->toGraphicsObject();
        const bool label = (object && object->inherits("UiTiGraphicsLabelItem")) ||
                           QByteArray(typeid(*item).name()).contains("UiTiGraphicsLabelItem");
        if (label) {
            const auto rect = item->sceneBoundingRect().intersected(bounds);
            if (!rect.isEmpty())
                result.append(rect.translated(-bounds.topLeft()));
        } else
            pending.append(item->childItems());
    }
    return result;
}
} // namespace Unbound
