// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include <QJsonDocument>
#include <optional>

namespace Unbound {
// Portable explicit clip assignment. The native clip owns association; runtime
// project/timeline/graphics IDs and personal-palette paths never enter the record.
struct ClipColorAssignment {
    ColorPreset paint;
    QString fallback;
    QByteArray encode() const {
        return QJsonDocument(QJsonObject{{"schema", 1}, {"scope", "explicit-clip"},
                                        {"paint", paint.json()}, {"fallback", fallback}})
            .toJson(QJsonDocument::Compact);
    }
    static std::optional<ClipColorAssignment> decode(const QByteArray &bytes) {
        if (bytes.isEmpty() || bytes.size() > 4096)
            return {};
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            return {};
        const auto object = document.object();
        ClipColorAssignment record;
        QString message;
        if (object["schema"] != 1 || object["scope"] != "explicit-clip" ||
            !object["paint"].isObject() ||
            !ColorPreset::parse(object["paint"].toObject(), record.paint, message))
            return {};
        record.fallback = object["fallback"].toString();
        if (record.fallback.isEmpty())
            return {};
        return record;
    }
};
} // namespace Unbound
