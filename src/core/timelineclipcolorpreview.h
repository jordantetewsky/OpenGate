// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "diagnostics.h"
#include "timelineclipcolorbindings.h"
#include "timelinecontentinspection.h"
#include "timelineclipselection.h"
#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QSet>
namespace Unbound {
// Resolve native Timeline selection to per-view color sessions; no paint logic.
// Persistent project association remains outside this temporary proof adapter.
class TimelineClipColorPreview : public QObject {
  public:
    TimelineClipColorPreview(QGraphicsView *view, QObject *owner, ColorPalette *palette = nullptr)
        : QObject(owner), view(view),
          bindings(view ? TimelineClipColorBindings::ensure(view) : nullptr) {
        if (palette)
            palette->subscribe(this, [session = bindings, palette] {
                if (session)
                    session->refreshDefinitions(palette->state().presets);
            });
    }
    ~TimelineClipColorPreview() override {
        clear();
    }
    bool hasSelection() const {
        for (auto surface : TimelineClipSelection::views(view))
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems())
                    if (TimelineClipSelection::container(item))
                        return true;
        return false;
    }
    int applySelected(QColor color) {
        if (!color.isValid())
            return 0;
        TimelineClipColorBindings::RepaintBatch repaint(bindings);
        int count = 0;
        for (const auto &clip : TimelineClipSelection::selected(view))
            if (bindings)
                count += bindings->apply(clip.surface, clip.object, color);
        logEvent("clip-color.preview.requested", {{"clips", count},
                                                  {"color", color.name()},
                                                  {"temporary", true},
                                                  {"project_writes", false}});
        return count;
    }
    int applySelected(const ColorPreset &style) {
        TimelineClipColorBindings::RepaintBatch repaint(bindings);
        int count = 0;
        int inspections = 0;
        const bool inspect = qEnvironmentVariableIntValue("OPENGATE_COLOR_PAINT_TRACE") == 1;
        const auto definition = QSharedPointer<const ColorPreset>::create(style);
        for (const auto &clip : TimelineClipSelection::selected(view)) {
            if (bindings)
                count += bindings->apply(clip.surface, clip.object, definition);
            if (inspect && inspections++ < 8)
                logEvent("clip-color.graphics-members",
                         TimelineContentInspection::captureClipMembers(clip.object));
        }
        logEvent("clip-color.style.requested", {{"clips", count},
                                                {"preset", style.id},
                                                {"texture", style.texture},
                                                {"stops", style.stops.size()},
                                                {"temporary", true},
                                                {"project_writes", false}});
        return count;
    }
    QString selectedPresetId() const {
        QString id;
        bool found = false;
        for (const auto &clip : TimelineClipSelection::selected(view)) {
            const auto next = bindings ? bindings->presetId(clip.object) : QString();
            if (next.isEmpty() || (found && id != next))
                return {};
            id = next;
            found = true;
        }
        return id;
    }
    void clear() {
        if (bindings)
            bindings->clear();
    }
    void clearSelected() {
        QSet<QGraphicsObject *> selected;
        for (const auto &clip : TimelineClipSelection::selected(view))
            selected.insert(clip.object);
        if (bindings && !selected.isEmpty())
            bindings->clear(selected);
    }

  private:
    QPointer<QGraphicsView> view;
    QPointer<TimelineClipColorBindings> bindings;
};
} // namespace Unbound
