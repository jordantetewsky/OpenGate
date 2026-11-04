// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "colorpalettedrafts.h"

namespace Unbound {
// Explicit interchange between the builder/native editor and the two existing
// palette/draft owners. This owns no state, dialogs or clip assignments.
class ColorPaletteTransfer {
  public:
    static QString apply(const QByteArray &bytes, ColorPalette &palette,
                         ColorPaletteDrafts &drafts) {
        ColorPalette::Data next, saved;
        auto error = palette.planImport(bytes, next);
        if (!error.isEmpty())
            return error;
        if (!ColorPalette::decode(bytes, saved, error))
            return error;
        const auto root = QJsonDocument::fromJson(bytes).object();
        QVector<ColorPreset> pending;
        if (root.contains("draftPresets")) {
            if (!root["draftPresets"].isArray())
                return "Invalid pending preset list.";
            auto draftDocument = root;
            draftDocument["presets"] = root["draftPresets"];
            ColorPalette::Data edited;
            if (!ColorPalette::decode(QJsonDocument(draftDocument).toJson(), edited, error))
                return error;
            for (const auto &draft : edited.presets) {
                const ColorPreset *base = nullptr;
                for (const auto &preset : saved.presets)
                    if (preset.id == draft.id) {
                        base = &preset;
                        break;
                    }
                if (!base)
                    return "A new color has not been saved in the builder. Use Update Preset for "
                           "that color, then export again; nothing was imported.";
                if (base->category != draft.category)
                    return "A pending preset has a different saved category; nothing was imported.";
                if (!(draft == *base))
                    pending.append(draft);
            }
        }
        if (!pending.isEmpty()) {
            error = drafts.appendImported(pending);
            if (!error.isEmpty())
                return error;
        }
        error = palette.replace(next);
        if (!error.isEmpty() && !pending.isEmpty())
            return "Pending edits were backed up; saved palette import failed: " + error;
        return error;
    }
    static QString encode(const ColorPalette &palette, const ColorPaletteDrafts &drafts,
                          QByteArray &bytes) {
        auto root = QJsonDocument::fromJson(ColorPalette::encode(palette.state())).object();
        QJsonArray pending;
        for (const auto &saved : palette.state().presets) {
            const auto draft = drafts.get(saved);
            if (!(draft == saved)) {
                ColorPreset validated;
                QString error;
                if (!ColorPreset::parse(draft.json(), validated, error))
                    return "Finish the pending preset before export: " + error;
                pending.append(draft.json());
            }
        }
        if (!pending.isEmpty())
            root["draftPresets"] = pending;
        const auto encoded = QJsonDocument(root).toJson();
        if (encoded.size() > 256000)
            return "Saved presets and pending edits exceed the 256 KB transfer limit.";
        bytes = encoded;
        return {};
    }
};
} // namespace Unbound
