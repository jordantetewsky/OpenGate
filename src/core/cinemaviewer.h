// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QPointer>
#include <QTimer>
#include "diagnostics.h"
#include "../platform/workspacewindowintegration.h"

namespace Unbound {
inline bool isCinemaViewer(QWidget *window)
{
    return window && window->isWindow() && (window->inherits("UiCinemaViewerDialog") ||
        window->objectName() == QLatin1String("UiCinemaViewerDialog"));
}
inline QWidget *visibleCinemaViewer()
{
    for (auto window : QApplication::topLevelWidgets()) if (window->isVisible() && isCinemaViewer(window)) return window;
    return nullptr;
}

// Yield panel ordering while the native viewer owns foreground input.
class CinemaViewerLifecycle : public QObject
{
public:
    explicit CinemaViewerLifecycle(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~CinemaViewerLifecycle() override
    {
        if (viewer) setNativeExpansionPriority(viewer, false);
        if (qApp) qApp->removeEventFilter(this);
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        const auto type = event->type();
        if (type != QEvent::FocusIn && type != QEvent::ApplicationStateChange && type != QEvent::Show && type != QEvent::Hide) return false;
        auto widget = qobject_cast<QWidget *>(object);
        if (event->type() == QEvent::FocusIn && widget && !isCinemaViewer(widget->window())) lastFocus = widget;
        if (event->type() == QEvent::ApplicationStateChange && viewer && viewer->isVisible()) {
            setNativeExpansionPriority(viewer, QGuiApplication::applicationState() == Qt::ApplicationActive);
        }
        if (!isCinemaViewer(widget)) return false;
        if (event->type() == QEvent::Show) {
            viewer = widget; previousFocus = lastFocus;
            QPointer<QWidget> alive(widget);
            QTimer::singleShot(0, this, [alive] {
                if (!alive || !alive->isVisible()) return;
                setNativeExpansionPriority(alive, true);
                alive->raise(); alive->activateWindow();
                logEvent(QStringLiteral("cinema.foreground.request"), {{"window", alive->objectName()}});
            });
        } else if (event->type() == QEvent::Hide && viewer == widget) {
            setNativeExpansionPriority(widget, false); viewer = nullptr;
            const auto focus = previousFocus; previousFocus = nullptr;
            QTimer::singleShot(0, this, [focus] {
                if (!focus || !focus->isVisible() || QGuiApplication::applicationState() != Qt::ApplicationActive || QApplication::activeModalWidget()) return;
                focus->window()->raise(); focus->window()->activateWindow(); focus->setFocus(Qt::OtherFocusReason);
            });
            logEvent(QStringLiteral("cinema.foreground.restore"));
        }
        return false;
    }
private:
    QPointer<QWidget> viewer, lastFocus, previousFocus;
};
}
