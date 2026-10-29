// SPDX-License-Identifier: MIT
#pragma once
#include <QGuiApplication>
#include <QScreen>
#include "displayplacement.h"
namespace Unbound {
inline QList<WorkspaceDisplay> workspaceDisplays() {
    QList<WorkspaceDisplay> result;
    if(!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))return result;
    for(auto s:QGuiApplication::screens())result.append({s->name(),s->serialNumber(),s->manufacturer(),s->model(),s->availableGeometry(),s==QGuiApplication::primaryScreen()});
    return result;
}
}
