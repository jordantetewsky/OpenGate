// SPDX-License-Identifier: MIT
#pragma once
#include "productidentity.h"
#include "../platform/diagnosticplatform.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>
#include <QHash>
#include <QTimer>
#include <QUuid>
#include <atomic>
#include "diagnosticsink.h"
#include "crashbreadcrumb.h"

#ifndef UNBOUND_BUILD_ID
#define UNBOUND_BUILD_ID "development"
#endif

namespace Unbound {
inline const QString &diagnosticSessionId()
{
    static const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return id;
}
inline void logEvent(const QString &event, QJsonObject details = {})
{
    const QString directory = diagnosticDirectory();
    static std::atomic<quint64> sequence{0};
    details.insert(QStringLiteral("session"), diagnosticSessionId());
    details.insert(QStringLiteral("event_sequence"), double(++sequence));
    details.insert(QStringLiteral("component"), productName());
    details.insert(QStringLiteral("vendor"), productVendor());
    details.insert(QStringLiteral("module"), diagnosticModule());
    details.insert(QStringLiteral("event"), event);
    details.insert(QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    details.insert(QStringLiteral("pid"), QCoreApplication::applicationPid());
    details.insert(QStringLiteral("build"), QStringLiteral(UNBOUND_BUILD_ID));
    details.insert(QStringLiteral("process"), QCoreApplication::applicationName());
    recordCrashBreadcrumb(event,details);
    writeDiagnostic(directory,event,QJsonDocument(details).toJson(QJsonDocument::Compact) + '\n');
}

inline QJsonObject widgetState(QWidget *widget,bool includeLayoutHints=true)
{
    if (!widget) return {{"exists", false}};
    const QRect r = widget->geometry();
    const auto minimumHint = includeLayoutHints ? widget->minimumSizeHint() : QSize(-1,-1);
    return {{"exists", true}, {"name", widget->objectName()},
        {"class", widget->metaObject()->className()},
        {"parent", widget->parentWidget() ? widget->parentWidget()->objectName() : QString()},
        {"window", widget->isWindow()}, {"visible", widget->isVisible()},
        {"hidden", widget->isHidden()}, {"enabled", widget->isEnabled()},
        {"focus_policy", int(widget->focusPolicy())}, {"window_type", int(widget->windowType())},
        {"focus_proxy", widget->focusProxy() ? widget->focusProxy()->objectName() : QString()},
        {"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()},
        {"minimum_width", widget->minimumWidth()}, {"minimum_height", widget->minimumHeight()},
        {"minimum_hint_width", minimumHint.width()},
        {"minimum_hint_height", minimumHint.height()}};
}

class WindowDiagnostics : public QObject
{
public:
    explicit WindowDiagnostics(QObject *parent) : QObject(parent)
    {
        geometryTimer.setSingleShot(true); geometryTimer.setInterval(100);
        connect(&geometryTimer, &QTimer::timeout, this, [this] { flushGeometry(); });
    }
    ~WindowDiagnostics() override { flushGeometry(); }
    void flushGeometry()
    {
        const auto pending = geometryChanges; geometryChanges.clear(); geometryTimer.stop();
        for (auto state : pending) {
            const auto event = state.take("geometry_event").toString(); logEvent(event, state);
        }
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        const char *action = nullptr;
        switch (event->type()) {
        case QEvent::Move: action = "window.move"; break;
        case QEvent::Resize: action = "window.resize"; break;
        case QEvent::Show: action = "window.show"; break;
        case QEvent::Hide: action = "window.hide"; break;
        case QEvent::Close: action = "window.close.request"; break;
        case QEvent::WindowStateChange: action = "window.state"; break;
        case QEvent::WindowActivate: action = "window.activate"; break;
        case QEvent::WindowDeactivate: action = "window.deactivate"; break;
        default: return false;
        }
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget || !widget->isWindow()) return false;
        const QString name = widget->objectName();
        if (name != QLatin1String("UiMainWindow") && name != QLatin1String("unboundPanelTest") &&
            !name.startsWith(QLatin1String("unboundFloating_"))) return false;
        if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
            const auto rect = widget->geometry();
            QJsonObject state{{"name",name},{"x",rect.x()},{"y",rect.y()},{"width",rect.width()},{"height",rect.height()},
                {"state",int(widget->windowState())},{"visible",widget->isVisible()}};
            state["coalesced_events"] = geometryChanges.value(widget).value("coalesced_events").toInt() + 1;
            state["geometry_event"] = action; geometryChanges[widget] = state;
            if (!geometryTimer.isActive()) geometryTimer.start();
            return false;
        }
        if (event->type() == QEvent::Hide || event->type() == QEvent::Close) flushGeometry();
        auto state = widgetState(widget); state["state"] = int(widget->windowState());
        logEvent(QLatin1String(action), state);
        return false;
    }
private:
    QTimer geometryTimer;
    QHash<QWidget *, QJsonObject> geometryChanges;
};
}
