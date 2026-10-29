// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include "../platform/workspaceprofile.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
namespace Unbound {
inline QString settingsProfilePath(const QString &file) {
    const auto base=workspaceProfileDirectory();
    QFile marker(qEnvironmentVariable("UNBOUND_PROFILE_FILE",base+"/Profile.json"));
    QString profile;
    if (marker.open(QIODevice::ReadOnly)) profile=QJsonDocument::fromJson(marker.readAll()).object()["active"].toString();
    return base+(profile=="first-install-preview" ? "/Profiles/first-install-preview/" : "/")+file;
}
}
