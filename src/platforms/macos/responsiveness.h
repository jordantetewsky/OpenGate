// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include "diagnostics.h"
#include "latencystats.h"
#include "nativechrome.h"
namespace Unbound {
// These are Qt dispatch/paint and event-loop measurements, not GPU presentation
// timestamps. Missing paints are reported separately, never counted as fast ones.
class ResponsivenessProbe : public QObject
{
public:
    explicit ResponsivenessProbe(QObject *owner) : QObject(owner)
    {
        clock.start(); lastTick = clock.nsecsElapsed();
        heartbeat.setInterval(16); heartbeat.setTimerType(Qt::PreciseTimer);
        connect(&heartbeat,&QTimer::timeout,this,[this] {
            const auto now = clock.nsecsElapsed();
            if (now < activeUntil) eventLoop.add(qMax(qint64(0),now-lastTick-16000000));
            lastTick = now;
        });
        reporter.setInterval(5000); connect(&reporter,&QTimer::timeout,this,[this] { flush(); });
        heartbeat.start(); reporter.start(); qApp->installEventFilter(this);
    }
    ~ResponsivenessProbe() override { if (qApp) qApp->removeEventFilter(this); flush(); }
    void flush()
    {
        if (eventLoop.empty() && paintDelay.empty() && layoutTurn.empty() && nativeAge.empty()) return;
        const auto now = clock.nsecsElapsed(); int withoutPaint = 0;
        for (auto item = awaitingPaint.begin(); item != awaitingPaint.end();) {
            if (!item->window || now-item->started > 1000000000) { ++withoutPaint; item = awaitingPaint.erase(item); }
            else ++item;
        }
        logEvent("ui.responsiveness", {{"event_loop_lateness",eventLoop.summary()},
            {"native_input_age",nativeAge.summary()},
            {"input_to_paint_dispatch",paintDelay.summary()}, {"layout_to_next_turn",layoutTurn.summary()},
            {"requests_without_observed_paint",withoutPaint}, {"scope","Qt dispatch; not GPU presentation"}});
        eventLoop.clear(); paintDelay.clear(); layoutTurn.clear(); nativeAge.clear();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        const auto type = event->type();
        if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease && type != QEvent::MouseMove &&
            type != QEvent::Resize && type != QEvent::Move && type != QEvent::KeyPress &&
            type != QEvent::LayoutRequest && type != QEvent::Paint) return false;
        auto widget = qobject_cast<QWidget *>(object); if (!widget) return false;
        auto window = widget->window(); const auto name = window->objectName();
        if (name != "unboundPanelTest" && !name.startsWith("unboundFloating_")) return false;
        const auto now = clock.nsecsElapsed();
        if (type == QEvent::Move) {
            if (widget != window) return false;
            const auto age = nativeCurrentDragAgeMilliseconds();
            if (age >= 0) { nativeAge.add(qint64(age*1000000)); activeUntil = now+1500000000; }
            return false;
        }
        if (type == QEvent::Paint) {
            auto pending = awaitingPaint.find(window);
            if (pending != awaitingPaint.end()) { paintDelay.add(now-pending->started); awaitingPaint.erase(pending); }
            return false;
        }
        if (type == QEvent::LayoutRequest) {
            if (pendingLayout.contains(window)) return false;
            pendingLayout.insert(window); QPointer<QWidget> alive(window);
            QTimer::singleShot(0,this,[this,alive,window,now] {
                pendingLayout.remove(window); if (alive) layoutTurn.add(clock.nsecsElapsed()-now);
            });
            return false;
        }
        if (type == QEvent::MouseMove && !static_cast<QMouseEvent *>(event)->buttons().testFlag(Qt::LeftButton)) return false;
        if (type == QEvent::Resize && widget != window) return false;
        if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease || type == QEvent::MouseMove || type == QEvent::KeyPress) {
            const auto age = nativeInputAgeMilliseconds(static_cast<QInputEvent *>(event)->timestamp());
            if (age >= 0) nativeAge.add(qint64(age*1000000));
        }
        activeUntil = now+1500000000;
        if (!awaitingPaint.contains(window) || !awaitingPaint.value(window).window || now-awaitingPaint.value(window).started > 1000000000)
            awaitingPaint[window] = {window,now};
        return false;
    }
private:
    struct Pending { QPointer<QWidget> window; qint64 started; };
    QElapsedTimer clock;
    QTimer heartbeat,reporter;
    qint64 lastTick = 0, activeUntil = 0;
    QHash<QWidget *,Pending> awaitingPaint;
    QSet<QWidget *> pendingLayout;
    LatencyStats eventLoop,paintDelay,layoutTurn,nativeAge;
};
}
