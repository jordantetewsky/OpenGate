// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimer>
namespace Unbound {
// Own only uncommitted editor drafts and their debounced local recovery file.
// Committed presets, interchange and project assignments have separate owners.
class ColorPaletteDrafts : public QObject {
  public:
    ColorPaletteDrafts(QObject *owner, QString location)
        : QObject(owner), path(std::move(location)) {
        timer.setSingleShot(true);
        timer.setInterval(180);
        connect(&timer, &QTimer::timeout, this, [this] { flush(); });
        QFile file(path);
        if (!file.exists())
            return;
        if (!file.open(QIODevice::ReadOnly) || file.size() > 256000) {
            problem = "Cannot read preset draft backup.";
            return;
        }
        QJsonParseError parse;
        const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
        const auto root = doc.object();
        if (parse.error != QJsonParseError::NoError ||
            root["schema"] != "com.cinewaltz.opengate.palette-drafts.v1" ||
            !root["drafts"].isArray() || root["drafts"].toArray().size() > 128) {
            problem = "Invalid preset draft backup; original retained.";
            return;
        }
        QHash<QString, ColorPreset> loaded;
        for (const auto &value : root["drafts"].toArray()) {
            ColorPreset p;
            QString error;
            if (!ColorPreset::parse(value.toObject(), p, error) || loaded.contains(p.id)) {
                problem = "Invalid preset draft backup; original retained.";
                return;
            }
            loaded.insert(p.id, p);
        }
        drafts = loaded;
    }
    ~ColorPaletteDrafts() override {
        if (timer.isActive())
            flush();
    }
    ColorPreset get(const ColorPreset &saved) const {
        auto draft = drafts.value(saved.id, saved);
        draft.category = saved.category;
        return draft;
    }
    void record(const ColorPreset &draft, const ColorPreset &saved) {
        if (draft == saved) {
            discard(draft.id);
            return;
        }
        if (drafts.contains(draft.id) && drafts[draft.id] == draft)
            return;
        drafts.insert(draft.id, draft);
        timer.start();
    }
    void discard(QString id) {
        if (drafts.remove(id))
            timer.start();
    }
    QString error() const {
        return problem;
    }
    // Explicit transfer flushes existing pending edits too, but never replaces
    // a different local draft with imported data.
    QString appendImported(const QVector<ColorPreset> &incoming) {
        if (!problem.isEmpty())
            return "Existing drafts need recovery before import: " + problem;
        auto next = drafts;
        for (const auto &preset : incoming) {
            if (next.contains(preset.id) && !(next[preset.id] == preset))
                return "A preset has different local pending edits; drafts were retained.";
            next.insert(preset.id, preset);
        }
        if (next == drafts)
            return {};
        if (next.size() > 128)
            return "Draft backup supports up to 128 colors.";
        const auto error = write(next);
        if (!error.isEmpty())
            return error;
        drafts = next;
        timer.stop();
        return {};
    }

  private:
    QString path, problem;
    QTimer timer;
    QHash<QString, ColorPreset> drafts;
    void flush() {
        timer.stop();
        if (!problem.isEmpty())
            return;
        problem = write(drafts);
    }
    QString write(const QHash<QString, ColorPreset> &valuesById) {
        QJsonArray values;
        auto keys = valuesById.keys();
        keys.sort();
        for (const auto &id : keys)
            values.append(valuesById[id].json());
        const auto bytes =
            QJsonDocument(QJsonObject{{"schema", "com.cinewaltz.opengate.palette-drafts.v1"},
                                      {"drafts", values}})
                .toJson();
        if (bytes.size() > 256000 || !QDir().mkpath(QFileInfo(path).absolutePath())) {
            return "Cannot save draft backup.";
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            return "Cannot save draft backup; committed presets retained.";
        return {};
    }
};
} // namespace Unbound
