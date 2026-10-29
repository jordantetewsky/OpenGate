// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QPointer>
#include <QHash>
#include <QElapsedTimer>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Inspect only registered group cells after layout settles. No global widget scan.
class BoundaryDiagnostics : public QObject {
public:
    explicit BoundaryDiagnostics(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~BoundaryDiagnostics() override { if (qApp) qApp->removeEventFilter(this); }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (event->type() != QEvent::Resize && event->type() != QEvent::Show && event->type() != QEvent::LayoutRequest) return false;
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget || !widget->objectName().startsWith("unboundGroupCell_")) return false;
        auto timer = timers.value(widget);
        if (!timer) {
            timer = new QTimer(this); timer->setSingleShot(true); timers[widget]=timer;
            QPointer<QWidget> alive(widget);
            connect(widget,&QObject::destroyed,this,[this,widget,timer] { timers.remove(widget); timer->deleteLater(); });
            connect(timer,&QTimer::timeout,this,[alive] { if (alive) audit(alive); });
        }
        timer->start(150); // A quiet interval, never a scan on each drag frame.
        return false;
    }
private:
    static void audit(QWidget *cell) {
        if (!cell->isVisible()) return;
        QElapsedTimer cost; cost.start(); QJsonArray overflow; int checked=0;
        QList<QWidget *> queue=cell->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly);
        while (!queue.isEmpty() && checked<512) {
            auto child=queue.takeLast(); ++checked;
            if (!child->isVisible() || child->isWindow()) continue;
            const QRect bounds(child->mapTo(cell,QPoint()),child->size());
            QString scrollViewport;
            for (auto parent=child->parentWidget(); parent && parent!=cell; parent=parent->parentWidget())
                if (parent->objectName().startsWith("unboundPanelOverflow_")) { scrollViewport=parent->objectName(); break; }
            if (!cell->rect().contains(bounds) && overflow.size()<16)
                overflow.append(QJsonObject{{"object",child->objectName()},{"class",child->metaObject()->className()},
                    {"x",bounds.x()},{"y",bounds.y()},{"width",bounds.width()},{"height",bounds.height()},
                    {"native",child->testAttribute(Qt::WA_NativeWindow)},{"paint_on_screen",child->testAttribute(Qt::WA_PaintOnScreen)},
                    {"overflow_viewport",scrollViewport}});
            queue.append(child->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly));
        }
        logEvent("panel.boundary.audit",{{"cell",cell->objectName()},{"width",cell->width()},
            {"height",cell->height()},{"descendants_checked",checked},{"truncated",!queue.isEmpty()},
            {"overflow",overflow},{"audit_ms",double(cost.nsecsElapsed())/1e6},
            {"scope","Geometry outside cell; does not by itself prove visible paint leakage"}});
    }
    QHash<QWidget *,QTimer *> timers;
};
}
