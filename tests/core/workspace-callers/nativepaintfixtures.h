// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsItem>
#include <QImage>
#include <QPainter>

namespace OpenGateTest {
// Model the observed native contract: positions follow the world transform,
// while glyph drawing uses device coordinates. This deliberately breaks a view
// transform and exercises the production backing-resolution rendering path.
class FixedGlyphItem : public QGraphicsItem {
public:
    QRectF boundingRect() const override { return {0, 0, 180, 60}; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        painter->fillRect(boundingRect(), Qt::black);
        const auto first = painter->worldTransform().map(QPointF(10, 20));
        const auto second = painter->worldTransform().map(QPointF(30, 20));
        painter->save(); painter->setWorldTransform(QTransform());
        QFont font; font.setPixelSize(16); painter->setFont(font); painter->setPen(Qt::red);
        painter->drawText(first, "H"); painter->drawText(second, "H"); painter->restore();
    }
};
inline int paintedInkHeight(const QImage &image, QColor target) {
    int first = image.height(), last = -1;
    for (int y = 0; y < image.height(); ++y) for (int x = 0; x < image.width(); ++x) {
        const auto color = image.pixelColor(x, y);
        if (qAbs(color.red()-target.red()) < 75 && qAbs(color.green()-target.green()) < 75 &&
            qAbs(color.blue()-target.blue()) < 75) {
            first = qMin(first, y); last = qMax(last, y);
        }
    }
    return last >= first ? last-first+1 : 0;
}
}
