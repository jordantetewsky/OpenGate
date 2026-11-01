// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QtMath>

namespace Unbound {
// Draw the existing scene at backing-store resolution, without a world-scale
// transform. Native glyph positions, glyphs and borders use one coordinate space.
inline QImage renderTimelineScene(QGraphicsScene *scene, const QRectF &source, double backingScale) {
    if (!scene || source.isEmpty() || !qIsFinite(backingScale) || backingScale <= 0. ||
        !qIsFinite(source.x()) || !qIsFinite(source.y()) ||
        !qIsFinite(source.width()) || !qIsFinite(source.height()))
        return {};
    const double width = source.width() * backingScale;
    const double height = source.height() * backingScale;
    if (width > 16384 || height > 16384 || width * height > 64. * 1024 * 1024)
        return {};
    QImage image(QSize(qCeil(width), qCeil(height)), QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return {};
    image.setDevicePixelRatio(backingScale);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    scene->render(&painter, QRectF(QPointF(), source.size()), source, Qt::IgnoreAspectRatio);
    return image;
}
}
