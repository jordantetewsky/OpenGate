// SPDX-License-Identifier: MIT
#pragma once
#include "settingsprofile.h"
#include <QApplication>
#include <QFileInfo>
#include <QPointer>
#include <QSaveFile>
#include <QVector>
#include <functional>
namespace Unbound {
// Own one local texture-display preference. No preset, clip assignment, project
// metadata or native editing-history writes occur here.
class ColorDisplaySettings : public QObject {
  public:
    static ColorDisplaySettings *shared() {
        static QPointer<ColorDisplaySettings> instance;
        if (!instance && qApp && !QCoreApplication::closingDown())
            instance = new ColorDisplaySettings(
                qApp, qEnvironmentVariable("OPENGATE_COLOR_DISPLAY_FILE",
                                           settingsProfilePath("ColorDisplay.json")));
        return instance;
    }
    ColorDisplaySettings(QObject *owner, QString location)
        : QObject(owner), path(std::move(location)) {
        QFile file(path);
        if (!file.exists())
            return;
        if (!file.open(QIODevice::ReadOnly) || file.size() > 4096) {
            problem = "Cannot read color display preferences.";
            return;
        }
        QJsonParseError parse;
        const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
        const auto root = doc.object();
        if (parse.error != QJsonParseError::NoError ||
            root["schema"] != "com.cinewaltz.opengate.color-display.v1" ||
            !root["textures"].isBool()) {
            problem = "Invalid color display preferences; original file retained.";
            return;
        }
        textures = root["textures"].toBool();
    }
    bool texturesEnabled() const {
        return textures;
    }
    QString error() const {
        return problem;
    }
    void subscribe(QObject *owner, std::function<void(bool)> changed) {
        listeners.append({owner, std::move(changed)});
    }
    QString setTexturesEnabled(bool value) {
        if (value == textures)
            return {};
        if (!problem.isEmpty())
            return problem;
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            return "Cannot create color display preference directory.";
        const auto bytes =
            QJsonDocument(QJsonObject{{"schema", "com.cinewaltz.opengate.color-display.v1"},
                                      {"textures", value}})
                .toJson();
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            return "Cannot save color display preferences; previous setting retained.";
        textures = value;
        const auto current = listeners;
        for (const auto &entry : current)
            if (entry.owner && entry.changed)
                entry.changed(value);
        for (int i = listeners.size() - 1; i >= 0; --i)
            if (!listeners[i].owner)
                listeners.removeAt(i);
        return {};
    }

  private:
    struct Listener {
        QPointer<QObject> owner;
        std::function<void(bool)> changed;
    };
    QVector<Listener> listeners;
    QString path, problem;
    bool textures = true;
};
} // namespace Unbound
