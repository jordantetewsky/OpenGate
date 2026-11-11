// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include "settingsprofile.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QPointer>
#include <QSaveFile>
#include <QUuid>
#include <functional>
#include <memory>
namespace Unbound {
// One owner of the personal palette, category ordering and validated interchange.
// This is deliberately not clip assignment or project-portable metadata storage.
class ColorPalette {
  public:
    ColorPalette(const ColorPalette &) = delete;
    ColorPalette &operator=(const ColorPalette &) = delete;
    struct Category {
        QString id, name;
    };
    struct Data {
        QVector<Category> categories;
        QVector<ColorPreset> presets;
    };
    explicit ColorPalette(QString location = {})
        : path(location.isEmpty() ? settingsProfilePath("Colors.json") : location) {
        data.categories = {{"custom", "Your Presets"}};
        ColorPreset red;
        red.id = "red";
        red.name = "Red";
        red.category = "custom";
        data.presets = {red};
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly) || file.size() > 256000)
                lastError = "Cannot read palette.";
            else {
                Data restored;
                if (decode(file.readAll(), restored, lastError))
                    data = restored;
            }
        }
    }
    const Data &state() const {
        return data;
    }
    quint64 revision() const {
        return version;
    }
    std::weak_ptr<const int> lifetime() const { return alive; }
    // Consumers observe committed definitions only. Weak owners make the
    // feature's model/widget destruction order independent of subscriptions.
    void subscribe(QObject *owner, std::function<void()> changed) {
        listeners.append({owner, std::move(changed)});
    }
    QString location() const {
        return path;
    }
    QString error() const {
        return lastError;
    }
    const ColorPreset *find(const QString &id) const {
        for (const auto &p : data.presets)
            if (p.id == id)
                return &p;
        return nullptr;
    }
    static QString newId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    static QByteArray encode(const Data &data) {
        QJsonArray categories, presets;
        for (const auto &c : data.categories)
            categories.append(QJsonObject{{"id", c.id}, {"name", c.name}});
        for (const auto &p : data.presets)
            presets.append(p.json());
        return QJsonDocument(QJsonObject{{"schema", "com.cinewaltz.opengate.palette-design.v2"},
                                         {"categories", categories},
                                         {"presets", presets}})
            .toJson();
    }
    static bool decode(const QByteArray &bytes, Data &result, QString &error) {
        const auto fail = [&](const QString &s) {
            error = s;
            return false;
        };
        if (bytes.size() > 256000)
            return fail("Palette exceeds 256 KB.");
        QJsonParseError parse;
        const auto doc = QJsonDocument::fromJson(bytes, &parse);
        if (parse.error != QJsonParseError::NoError || !doc.isObject())
            return fail("Invalid palette JSON.");
        const auto root = doc.object();
        if (root["schema"] != "com.cinewaltz.opengate.palette-design.v2" &&
            root["schema"] != "com.cinewaltz.opengate.palette-design.v1")
            return fail("Unsupported palette version.");
        if (!root["presets"].isArray() || root["presets"].toArray().size() > 128)
            return fail("A palette supports up to 128 editable colors.");
        Data next;
        QSet<QString> ids;
        auto categories = root["categories"].toArray();
        if (categories.isEmpty())
            categories = {QJsonObject{{"id", "proposed"}, {"name", "OpenGate"}},
                          QJsonObject{{"id", "gradient"}, {"name", "Gradients"}},
                          QJsonObject{{"id", "custom"}, {"name", "Your Presets"}}};
        if (categories.size() > 32)
            return fail("A palette supports up to 32 categories.");
        const QRegularExpression pattern("^[A-Za-z0-9_-]{1,100}$");
        for (const auto &value : categories) {
            const auto o = value.toObject();
            Category c{o["id"].toString(), o["name"].toString().trimmed()};
            if (!pattern.match(c.id).hasMatch() || c.id == "stock" || c.name.isEmpty() ||
                c.name.size() > 80 || ids.contains(c.id))
                return fail("Invalid or duplicate category.");
            ids.insert(c.id);
            next.categories.append(c);
        }
        QSet<QString> colors;
        for (const auto &value : root["presets"].toArray()) {
            ColorPreset p;
            if (!value.isObject() || !ColorPreset::parse(value.toObject(), p, error))
                return false;
            if (colors.contains(p.id) || !ids.contains(p.category))
                return fail("Duplicate color or missing category.");
            colors.insert(p.id);
            next.presets.append(p);
        }
        result = next;
        error.clear();
        return true;
    }
    QString replace(const Data &next) {
        Data validated;
        QString problem;
        const auto bytes = encode(next);
        if (!decode(bytes, validated, problem))
            return problem;
        if (!lastError.isEmpty())
            return "Existing palette needs recovery before it can be overwritten: " + lastError;
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            return "Cannot create palette directory.";
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            return "Cannot save palette; existing data retained.";
        data = validated;
        ++version;
        const auto current = listeners;
        for (const auto &entry : current)
            if (entry.owner && entry.changed)
                entry.changed();
        for (int i = listeners.size() - 1; i >= 0; --i)
            if (!listeners[i].owner)
                listeners.removeAt(i);
        return {};
    }
    QString update(const ColorPreset &preset) {
        auto next = data;
        bool found = false;
        for (auto &p : next.presets)
            if (p.id == preset.id) {
                p = preset;
                found = true;
                break;
            }
        if (!found)
            next.presets.append(preset);
        return replace(next);
    }
    QString import(const QByteArray &bytes) {
        Data next;
        const auto problem = planImport(bytes, next);
        return problem.isEmpty() ? replace(next) : problem;
    }
    QString planImport(const QByteArray &bytes, Data &result) const {
        if (!lastError.isEmpty())
            return "Existing palette needs recovery before import: " + lastError;
        Data incoming;
        QString problem;
        if (!decode(bytes, incoming, problem))
            return problem;
        auto next = data;
        for (const auto &category : incoming.categories) {
            bool found = false;
            for (const auto &existing : next.categories)
                if (existing.id == category.id) {
                    if (existing.name != category.name)
                        return "A category ID has a different existing name; the palette was not "
                               "changed.";
                    found = true;
                    break;
                }
            if (!found)
                next.categories.append(category);
        }
        for (const auto &preset : incoming.presets) {
            bool found = false;
            for (const auto &existing : next.presets)
                if (existing.id == preset.id) {
                    if (!(existing == preset))
                        return "A preset ID has different existing settings; the palette was not "
                               "changed.";
                    found = true;
                    break;
                }
            if (!found)
                next.presets.append(preset);
        }
        Data validated;
        if (!decode(encode(next), validated, problem))
            return problem;
        result = validated;
        return {};
    }

  private:
    const std::shared_ptr<const int> alive = std::make_shared<const int>(0);
    QString path, lastError;
    Data data;
    struct Listener {
        QPointer<QObject> owner;
        std::function<void()> changed;
    };
    QVector<Listener> listeners;
    quint64 version = 0;
};
} // namespace Unbound
