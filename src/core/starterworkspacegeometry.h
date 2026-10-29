// SPDX-License-Identifier: MIT
#pragma once
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QScreen>
namespace Unbound {
inline QJsonObject fitStarterWorkspace(QJsonObject value) {
    if (!value["fit_first_display"].toBool() || !QGuiApplication::primaryScreen()) return value;
    const auto frame=QGuiApplication::primaryScreen()->availableGeometry(); QJsonArray groups;
    for (auto item:value["groups"].toArray()) {
        auto group=item.toObject(); group["x"]=frame.x();group["y"]=frame.y();
        group["width"]=frame.width();group["height"]=frame.height();groups.append(group);
    }
    value["groups"]=groups; value.remove("fit_first_display"); return value;
}
}
