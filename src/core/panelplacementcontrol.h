// SPDX-License-Identifier: MIT
#pragma once
#include <QIconEngine>
#include <QPainter>
#include <QToolButton>
namespace Unbound {
// OpenGate's open gate and directional arrow; no overlapping-window emblem.
class PanelPlacementGlyph : public QIconEngine {
public:
    explicit PanelPlacementGlyph(bool floating=false):floating(floating) {}
    QIconEngine *clone() const override { return new PanelPlacementGlyph(floating); }
    void paint(QPainter *p,const QRect &r,QIcon::Mode mode,QIcon::State) override {
        p->save();p->setRenderHint(QPainter::Antialiasing);p->translate(r.topLeft());p->scale(r.width()/20.,r.height()/20.);
        p->setPen(QPen(mode==QIcon::Active ? QColor(230,230,230) : QColor(160,160,160),1.5,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p->drawLine(QPointF(3,4),QPointF(3,17));p->drawLine(QPointF(3,17),QPointF(17,17));p->drawLine(QPointF(17,17),QPointF(17,12));
        if (floating) { p->drawLine(QPointF(16,3),QPointF(8,11));p->drawLine(QPointF(8,6),QPointF(8,11));p->drawLine(QPointF(8,11),QPointF(13,11)); }
        else { p->drawLine(QPointF(8,11),QPointF(16,3));p->drawLine(QPointF(11,3),QPointF(16,3));p->drawLine(QPointF(16,3),QPointF(16,8)); }
        p->restore();
    }
    QPixmap pixmap(const QSize &size,QIcon::Mode mode,QIcon::State state) override { QPixmap result(size);result.fill(Qt::transparent);QPainter p(&result);paint(&p,QRect(QPoint(),size),mode,state);return result; }
private:bool floating;
};
inline void setPlacementControl(QToolButton *button,bool floating,const QString &description) {
    button->setText({});button->setIcon(QIcon(new PanelPlacementGlyph(floating)));button->setIconSize(QSize(18,18));
    button->setToolTip(description);button->setAccessibleName(description);button->setProperty("openGateChrome",true);
}
}
