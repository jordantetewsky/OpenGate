// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
#include <QPainter>
#include "workspaceframepolicy.h"
namespace Unbound {
class DockCompass : public QWidget
{
public:
    explicit DockCompass(QWidget *owner) : QWidget(owner, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus)
    {
        setObjectName("unboundDockCompass"); setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents); setAttribute(Qt::WA_TranslucentBackground);
    }
    void display(const QList<DockZone> &value, QPoint pointer)
    {
        zones = value; hover = pointer;
        QRect bounds; for (const auto &zone : zones) bounds = bounds.united(zone.target);
        setGeometry(bounds.adjusted(-2, -2, 2, 2)); show(); raise(); update();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        for (const auto &zone : zones) {
            const QRect box = zone.target.translated(-geometry().topLeft());
            painter.setPen(QColor("#80a5c9")); painter.setBrush(zone.target.contains(hover) ? QColor("#527da5") : QColor("#273849"));
            painter.drawRoundedRect(box, 4, 4); painter.setPen(Qt::white);
            const QString label = zone.edge == Qt::Edge(0) ? QString::fromUtf8("▣") : zone.edge == Qt::LeftEdge ? QString::fromUtf8("←") :
                zone.edge == Qt::RightEdge ? QString::fromUtf8("→") : zone.edge == Qt::TopEdge ? QString::fromUtf8("↑") : QString::fromUtf8("↓");
            painter.drawText(box, Qt::AlignCenter, label);
        }
    }
private:
    QList<DockZone> zones;
    QPoint hover;
};

}
