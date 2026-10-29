// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QHash>
#include <QJsonArray>
#include <QTimer>
#include <algorithm>
#include "diagnostics.h"
#include "../platform/nativewindowcontract.h"
#include "cinemaviewer.h"
#include "uiworktiming.h"
#include "windowstackpolicy.h"

namespace Unbound {
class WindowHierarchy : public QObject
{
public:
    explicit WindowHierarchy(QObject *parent) : QObject(parent)
    {
        new CinemaViewerLifecycle(this);
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, this, [this] {
            if (QGuiApplication::applicationState() == Qt::ApplicationActive) apply();
        });
        qApp->installEventFilter(this);
    }
    ~WindowHierarchy() override { qApp->removeEventFilter(this); }
    void setEnabled(bool value) { enabled = value; if (value) timer.start(0); }
    void apply()
    {
        if (!enabled || ordering || QApplication::activeModalWidget() || QApplication::activePopupWidget()) return;
        if (visibleCinemaViewer()) return;
        UiWorkTiming timing("window.order",nullptr);
        QList<QWidget *> floating;
        QWidget *main = nullptr, *controls = nullptr;
        for (auto widget : QApplication::topLevelWidgets()) {
            if (!widget->isVisible() || widget->isMinimized()) continue;
            if (widget->objectName().startsWith(QLatin1String("unboundFloating_"))) floating.append(widget);
            if (widget->objectName() == QLatin1String("UiMainWindow")) main = widget;
            if (widget->objectName() == QLatin1String("unboundPanelTest")) controls = widget;
        }
        if (floating.isEmpty()) return;
        for (auto widget : floating) if (!widget->property("unboundExpanded").toBool()) normalizeNativePanel(widget);
        const bool pinned = controls && controls->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
        if (controls && !pinned) floating.append(controls);
        std::stable_sort(floating.begin(), floating.end(), [this](QWidget *a, QWidget *b) {
            const int left = a->objectName() == "unboundPanelTest" && QApplication::activeWindow() == a ? 4 : workspaceWindowTier(a);
            const int right = b->objectName() == "unboundPanelTest" && QApplication::activeWindow() == b ? 4 : workspaceWindowTier(b);
            return left != right ? left < right : activation.value(a) < activation.value(b);
        });
        QList<QWidget *> windows;
        if (main && main->property("unboundBackgroundHidden").toBool()) setNativeBackgroundHidden(main, floating, true);
        else if (main) windows.append(main);
        windows.append(floating);
        if (controls && pinned) windows.append(controls);
        for (int i = 0; i < windows.size(); ++i) if (windows[i]->property("unboundExpanded").toBool()) {
            auto expanded = windows.takeAt(i); windows.append(expanded); break;
        }
        const qint64 key = nativeKeyWindowNumber();
        ordering = true; orderNativeWindows(windows); ordering = false;
        QJsonArray names,identities;
        for (auto widget : windows) {
            names.append(widget->objectName());
            identities.append(QJsonObject{{"name",widget->objectName()},{"native_id",nativeWindowNumber(widget)},
                {"tier",workspaceWindowTier(widget)},{"pinned",widget->property("unboundPinned").toBool()}});
        }
        logEvent(QStringLiteral("window.order"), {{"bottom_to_top", names}, {"identities",identities}, {"key_before", key},
            {"key_after", nativeKeyWindowNumber()}, {"focus_preserved", key == nativeKeyWindowNumber()}});
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!enabled || ordering) return false;
        const auto type = event->type();
        if (type != QEvent::ApplicationStateChange && type != QEvent::WindowActivate && type != QEvent::DynamicPropertyChange &&
            type != QEvent::Show && type != QEvent::Hide && type != QEvent::WindowStateChange && type != QEvent::ZOrderChange) return false;
        if (event->type() == QEvent::ApplicationStateChange) { timer.start(0); return false; }
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget || !widget->isWindow()) return false;
        const QString name = widget->objectName();
        if (!name.startsWith(QLatin1String("unboundFloating_")) && name != QLatin1String("UiMainWindow") &&
            name != QLatin1String("unboundPanelTest")) {
            if (event->type() == QEvent::WindowActivate || (isCinemaViewer(widget) && event->type() == QEvent::Hide)) timer.start(0);
            return false;
        }
        switch (event->type()) {
        case QEvent::WindowActivate:
            if (!activation.contains(widget)) connect(widget, &QObject::destroyed, this, [this, widget] { activation.remove(widget); });
            activation[widget] = ++serial;
            timer.start(0); break;
        case QEvent::DynamicPropertyChange:
        case QEvent::Show:
        case QEvent::Hide:
        case QEvent::WindowStateChange:
        case QEvent::ZOrderChange:
            timer.start(0); break;
        default: break;
        }
        return false;
    }
private:
    QTimer timer;
    QHash<QWidget *, quint64> activation;
    quint64 serial = 0;
    bool enabled = true, ordering = false;
};
}
