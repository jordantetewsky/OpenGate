// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
namespace Unbound {
// Seed optional records only. The immutable first-open layout comes from the
// shared Resolve Layout recipe; existing arrangements are never overwritten.
inline QString seedBundledPresets(const QString &destination,const QString &bundlePath) {
    if(QFileInfo::exists(destination))return {};
    QFile bundle(bundlePath);
    if(!bundle.open(QIODevice::ReadOnly))return QStringLiteral("Cannot read bundled workspace presets.");
    QJsonParseError error;
    const auto value=QJsonDocument::fromJson(bundle.readAll(),&error).object();
    if(error.error!=QJsonParseError::NoError||!value["arrangements"].isArray())return QStringLiteral("Bundled workspace presets are invalid.");
    QJsonArray records;
    for(const auto &item:value["arrangements"].toArray())if(!item.toObject()["read_only"].toBool())records.append(item);
    const auto directory=QFileInfo(destination).absolutePath();
    if(!QDir().mkpath(directory))return QStringLiteral("Cannot create the workspace profile directory.");
    QTemporaryFile staged(QDir(directory).filePath("preset-seed-XXXXXX"));
    const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"arrangements",records}}).toJson();
    if(!staged.open()||staged.write(bytes)!=bytes.size()||!staged.flush())return QStringLiteral("Cannot stage bundled workspace presets.");
    staged.close();
    // QFile::copy refuses an existing destination, including one created while
    // staging. Never replace an intervening save from another runtime.
    if(QFile::copy(staged.fileName(),destination)||QFileInfo::exists(destination))return {};
    return QStringLiteral("Cannot seed bundled workspace presets.");
}
}
