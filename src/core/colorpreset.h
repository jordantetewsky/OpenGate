// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QVector>
#include <algorithm>
#include <cmath>
namespace Unbound {
// Portable, bounded paint definitions. No widgets, assignment or project writes.
struct ColorStop {
    QString id;
    QColor color;
    double position = 0, opacity = 1;
};
struct ColorPreset {
    QString id, name, category, texture = "flat";
    QColor color = Qt::red;
    QVector<ColorStop> stops;
    double opacity = 1, strength = .45, angle = 90;
    bool operator==(const ColorPreset &other) const {
        return json() == other.json();
    }
    static QStringList textures() {
        return {"flat",           "grain",
                "grain-medium",   "grain-coarse",
                "stipple-fine",   "stipple",
                "stipple-coarse", "crosshatch-fine",
                "crosshatch",     "crosshatch-coarse",
                "brushed-fine",   "brushed",
                "brushed-coarse"};
    }
    static QStringList textureNames() {
        return {"Flat paint",        "Fine grain",        "Medium grain",   "Coarse grain",
                "Fine stipple",      "Medium stipple",    "Coarse stipple", "Fine crosshatch",
                "Medium crosshatch", "Coarse crosshatch", "Fine brushed",   "Medium brushed",
                "Coarse brushed"};
    }
    QJsonObject json() const {
        QJsonObject o{{"id", id},
                      {"name", name},
                      {"category", category},
                      {"hex", (stops.isEmpty() ? color : stops.front().color).name(QColor::HexRgb)},
                      {"opacity", opacity * 100},
                      {"finish", texture},
                      {"finishStrength", strength * 100},
                      {"angle", angle}};
        if (!stops.isEmpty()) {
            QJsonArray a;
            for (const auto &s : stops)
                a.append(QJsonObject{{"id", s.id},
                                     {"hex", s.color.name(QColor::HexRgb)},
                                     {"position", s.position * 100},
                                     {"opacity", s.opacity * 100}});
            o["stops"] = a;
            o["end"] = stops.last().color.name();
        }
        return o;
    }
    static bool parse(const QJsonObject &o, ColorPreset &result, QString &error) {
        const auto fail = [&](const QString &s) {
            error = s;
            return false;
        };
        ColorPreset p;
        const QRegularExpression idPattern("^[A-Za-z0-9_-]{1,100}$"), hex("^#[A-Fa-f0-9]{6}$");
        p.id = o["id"].toString();
        p.name = o["name"].toString().trimmed();
        p.category = o["category"].toString(o["group"].toString("custom"));
        if (!idPattern.match(p.id).hasMatch() || p.id.startsWith("native-") ||
            !idPattern.match(p.category).hasMatch())
            return fail("Invalid preset or category ID.");
        if (p.name.isEmpty() || p.name.size() > 80)
            return fail("Preset names need 1–80 characters.");
        if (!hex.match(o["hex"].toString()).hasMatch())
            return fail("Use a six-digit RGB hex color.");
        p.color = QColor(o["hex"].toString());
        auto number = [&](const QJsonObject &v, const QString &key, double fallback, double low,
                          double high, double &out) {
            const auto n = v[key];
            out = n.isUndefined() ? fallback : n.toDouble(NAN);
            return std::isfinite(out) && out >= low && out <= high;
        };
        if (!number(o, "opacity", 100, 0, 100, p.opacity) ||
            !number(o, "finishStrength", 45, 0, 100, p.strength) ||
            !number(o, "angle", 90, 0, 360, p.angle))
            return fail("Paint values are out of range.");
        p.opacity /= 100;
        p.strength /= 100;
        p.texture = o["finish"].toString("flat");
        if (!textures().contains(p.texture))
            return fail("Unsupported texture; the original file was not changed.");
        if (o.contains("stops")) {
            if (!o["stops"].isArray() || o["stops"].toArray().size() < 2 ||
                o["stops"].toArray().size() > 5)
                return fail("Gradients need 2–5 stops.");
            QSet<QString> ids;
            for (const auto &entry : o["stops"].toArray()) {
                const auto s = entry.toObject();
                ColorStop stop;
                stop.id = s["id"].toString(p.id + "-stop-" + QString::number(p.stops.size()));
                if (stop.id.size() > 140 || stop.id.isEmpty() || ids.contains(stop.id) ||
                    !hex.match(s["hex"].toString()).hasMatch() ||
                    !number(s, "position", 0, 0, 100, stop.position) ||
                    !number(s, "opacity", 100, 0, 100, stop.opacity))
                    return fail("Invalid gradient stop.");
                ids.insert(stop.id);
                stop.color = QColor(s["hex"].toString());
                stop.position /= 100;
                stop.opacity /= 100;
                p.stops.append(stop);
            }
            std::stable_sort(p.stops.begin(), p.stops.end(),
                             [](const auto &a, const auto &b) { return a.position < b.position; });
            p.color = p.stops.front().color;
        } else if (o.contains("end")) {
            if (!hex.match(o["end"].toString()).hasMatch())
                return fail("Invalid gradient end color.");
            p.stops = {{p.id + "-start", p.color, 0, 1},
                       {p.id + "-end", QColor(o["end"].toString()), 1, 1}};
        }
        result = p;
        error.clear();
        return true;
    }
};
} // namespace Unbound
