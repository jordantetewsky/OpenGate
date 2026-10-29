// SPDX-License-Identifier: MIT
#pragma once
#include <QCoreApplication>
#include "../platform/diagnosticplatform.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSaveFile>
namespace Unbound {
// Ownership boundaries only, never pointer-move telemetry or a signal handler.
inline void recordCrashBreadcrumb(const QString &event,const QJsonObject &record) {
    if (!event.startsWith("workspace.edit.") && event!="panel.undock.request" && event!="panel.undock.complete" &&
        event!="panel.dock.request" && event!="panel.dock.complete" && event!="group.detach" &&
        !event.startsWith("accessory.") && !event.startsWith("workspace.page.") &&
        !event.startsWith("header.pinned.") && !event.startsWith("footer.visibility.")) return;
    const auto path=diagnosticDirectory()+"/active-session.json";
    QFile input(path);if (!input.open(QIODevice::ReadOnly)) return;
    auto session=QJsonDocument::fromJson(input.readAll()).object();input.close();
    if (session["pid"].toDouble()!=QCoreApplication::applicationPid()) return;
    auto history=session["ownership_breadcrumbs"].toArray();history.append(record);
    while (history.size()>12) history.removeAt(0);
    session["ownership_breadcrumbs"]=history;QSaveFile output(path);
    if (output.open(QIODevice::WriteOnly)) { output.write(QJsonDocument(session).toJson(QJsonDocument::Compact));output.commit(); }
}
}
