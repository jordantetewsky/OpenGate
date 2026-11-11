// SPDX-License-Identifier: MIT
#pragma once
#include "clipcolorassignment.h"
#include "clipcolorassignmentbackend.h"
#include "clipcolorfallback.h"
#include "colorpalette.h"
#include "diagnostics.h"
#include "nativecommandactivity.h"
#include "timelineclipselection.h"

namespace Unbound {
// Translate one committed palette edit/delete into one document operation for
// registered loaded views. The backend owns saved records; this holds only the
// prior palette revision and weak view registrations, never a second assignment
// map. Enumerating assignments in unopened native timelines remains a gate.
class ClipColorDefinitionUpdates : public QObject {
  public:
    static void attach(ColorPalette &palette, ClipColorAssignmentBackend &document, QGraphicsView *root) {
        ClipColorDefinitionUpdates *updates = nullptr;
        const auto children = document.children();
        for (auto child : children) {
            if (auto candidate = dynamic_cast<ClipColorDefinitionUpdates *>(child)) {
                if (candidate->paletteLife.expired()) delete candidate;
                else if (&candidate->palette == &palette) { updates = candidate; break; }
            }
        }
        if (!updates) updates = new ClipColorDefinitionUpdates(palette, document);
        if (!updates->roots.contains(root)) updates->roots.append(root);
    }
  private:
    ColorPalette &palette;
    std::weak_ptr<const int> paletteLife;
    ClipColorAssignmentBackend &document;
    QVector<ColorPreset> previous;
    QList<QPointer<QGraphicsView>> roots;
    ClipColorDefinitionUpdates(ColorPalette &palette, ClipColorAssignmentBackend &document)
        : QObject(&document), palette(palette), paletteLife(palette.lifetime()), document(document), previous(palette.state().presets) {
        palette.subscribe(this, [this] { committed(); });
    }
    void committed() {
        if (paletteLife.expired()) return;
        QVector<ClipColorAssignmentBackend::Edit> edits;
        QSet<QGraphicsObject *> seen;
        for (auto root : roots)
            for (auto surface : TimelineClipSelection::views(root))
                if (surface->scene())
                    for (auto item : surface->scene()->items())
                        if (auto clip = TimelineClipSelection::container(item)) {
                            if (seen.contains(clip)) continue;
                            seen.insert(clip);
                            const auto before = document.read(clip);
                            const auto assignment = before.available && before.present
                                ? ClipColorAssignment::decode(before.bytes) : std::optional<ClipColorAssignment>{};
                            if (!assignment) continue;
                            auto native = before.nativeColor;
                            if (native.startsWith("ClipColor")) native.remove(0, 9);
                            if (native != assignment->fallback) continue;
                            const ColorPreset *old = nullptr;
                            for (const auto &preset : previous) if (preset.id == assignment->paint.id) old = &preset;
                            const auto replacement = palette.find(assignment->paint.id);
                            if (!old || (replacement && *old == *replacement)) continue;
                            const auto fallback = replacement ? ClipColorFallback::nearest(*replacement) : assignment->fallback;
                            const auto bytes = replacement ? ClipColorAssignment{*replacement, fallback}.encode() : QByteArray();
                            edits.append({clip, before, replacement != nullptr, bytes, fallback});
                        }
        if (!edits.isEmpty()) {
            if (!document.commit(edits, "Update Assigned Clip Colors"))
                logEvent("clip-color.definition-update.failed", {{"clips", edits.size()}});
            if (auto activity = NativeCommandActivity::shared()) activity->completed("clip-color.palette-update");
        }
        previous = palette.state().presets;
        for (int i = roots.size()-1; i >= 0; --i) if (!roots[i]) roots.removeAt(i);
    }
};
} // namespace Unbound
