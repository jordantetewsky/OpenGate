// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QtMath>
#include <QVector>
#include <algorithm>

namespace Unbound {
// Draw the existing scene at backing-store resolution, without a world-scale
// transform. Native glyph positions, glyphs and borders use one coordinate space.
inline QImage renderTimelineScene(QGraphicsScene *scene, const QRectF &source, double backingScale,
                                  const QVector<double> &sectionTops = {}) {
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
    if(sectionTops.isEmpty()) {
        scene->render(&painter, QRectF(QPointF(), source.size()), source, Qt::IgnoreAspectRatio);
        return image;
    }
    QVector<double> edges{0.,source.height()};
    for(auto top:sectionTops)if(qIsFinite(top) && top>0. && top<source.height())edges.append(top);
    std::sort(edges.begin(),edges.end());
    edges.erase(std::unique(edges.begin(),edges.end()),edges.end());
    // Each native section gets a bounded raster device. A native GPU/custom
    // painter that drops its clip cannot write into another section's image.
    for(int i=1;i<edges.size();++i) {
        const double top=edges[i-1], height=edges[i]-top;
        QImage band(QSize(image.width(),qCeil(height*backingScale)),image.format());
        if(band.isNull())return {};
        band.setDevicePixelRatio(backingScale);band.fill(Qt::transparent);
        {
            QPainter native(&band);
            scene->render(&native,QRectF(0,0,source.width(),height),
                          QRectF(source.x(),source.y()+top,source.width(),height),Qt::IgnoreAspectRatio);
        }
        painter.save();painter.setClipRect(QRectF(0,top,source.width(),height));
        painter.drawImage(QPointF(0,top),band);painter.restore();
    }
    return image;
}
}
