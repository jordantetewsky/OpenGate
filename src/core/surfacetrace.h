// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include <cmath>
#include "diagnostics.h"
#include "../platform/diagnosticplatform.h"
namespace Unbound {
// Bounded pointer samples, separate from action logs. Never consumes input.
class SurfaceTrace : public QObject {
public:
    explicit SurfaceTrace(QObject *owner) : QObject(owner) {
        clock.start(); timer.setInterval(250);
        connect(&timer,&QTimer::timeout,this,[this] { flush(); });
        timer.start(); qApp->installEventFilter(this);
    }
    ~SurfaceTrace() override { if (qApp) qApp->removeEventFilter(this); flush(); }
    void flush() {
        if (samples.isEmpty()) return;
        logEvent("input.pointer.batch",{{"samples",samples},{"coalesced_moves",coalesced},
            {"scope","Qt receipt and next event turn; not frame presentation"}});
        samples = {}; coalesced = 0;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        const auto type = event->type();
        if (type != QEvent::MouseMove && type != QEvent::MouseButtonPress &&
            type != QEvent::MouseButtonRelease && type != QEvent::MouseButtonDblClick) return false;
        auto widget = qobject_cast<QWidget *>(object); if (!widget || !related(widget)) return false;
        auto mouse = static_cast<QMouseEvent *>(event); const auto now = clock.nsecsElapsed();
        if (type == QEvent::MouseMove && lastSample >= 0 && now-lastSample < 16000000) { ++coalesced; return false; }
        const auto point = mouse->globalPos();
        QJsonObject record{{"t_ns",double(now)},{"event",int(type)},
            {"receiver",widget->objectName()},{"class",widget->metaObject()->className()},
            {"window",widget->window()->objectName()},{"x",point.x()},{"y",point.y()},
            {"buttons",int(mouse->buttons())},{"accepted_at_observation",event->isAccepted()}};
        if (lastSample >= 0 && now > lastSample) {
            const auto delta = point-lastPoint;
            record["speed_logical_px_per_s"] = std::hypot(double(delta.x()),double(delta.y()))*1e9/double(now-lastSample);
        }
        if (mouse->timestamp()) {
            const auto age = diagnosticInputAgeMilliseconds(mouse->timestamp());
            if (age >= 0) record["native_age_ms"] = age;
        }
        lastSample = now; lastPoint = point;
        if (samples.size() >= 256) flush();
        samples.append(record);
        if (type != QEvent::MouseMove) {
            flush();
            QPointer<QWidget> alive(widget);
            QTimer::singleShot(0,this,[this,alive,now,type] {
                logEvent("input.surface.next-turn",{{"receiver",alive ? alive->objectName() : QString()},
                    {"event",int(type)},{"elapsed_ms",double(clock.nsecsElapsed()-now)/1e6},
                    {"receiver_alive",bool(alive)}});
            });
        }
        return false;
    }
private:
    static bool related(QWidget *widget) {
        const auto top = widget->window()->objectName();
        if (top == "UiMainWindow" || top == "unboundPanelTest" || top.startsWith("unboundFloating_")) return true;
        for (auto owner=widget; owner; owner=owner->parentWidget())
            if (owner->objectName() == "unboundPanelControls") return true;
        return false;
    }
    QElapsedTimer clock; QTimer timer; QJsonArray samples;
    qint64 lastSample = -1; QPoint lastPoint; int coalesced = 0;
};
}
