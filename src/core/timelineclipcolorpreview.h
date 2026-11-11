// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "clipcolorassignment.h"
#include "clipcolorfallback.h"
#include "clipcolordefinitionupdates.h"
#include "../platform/clipcolorassignmentbackend.h"
#include "diagnostics.h"
#include "timelineclipcolorbindings.h"
#include "timelinecontentinspection.h"
#include "timelineclipselection.h"
#include "timelineclipcolorrestore.h"
#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QSet>
namespace Unbound {
// Coordinate native selection with document assignments and disposable display
// bindings. Native storage/history and painting stay in their focused adapters.
class TimelineClipColorPreview : public QObject {
  public:
    TimelineClipColorPreview(QGraphicsView *view, QObject *owner, ColorPalette *palette = nullptr,
                             ClipColorAssignmentBackend *document = nullptr)
        : QObject(owner), view(view),
          bindings(view ? TimelineClipColorBindings::ensure(view) : nullptr),
          document(document ? document : platformClipColorAssignmentBackend()) {
        setObjectName("openGateTimelineClipColors");
        if (this->document)
            restore = new TimelineClipColorRestore(view, this, [this] { reload(); });
        setPalette(palette);
    }
    static TimelineClipColorPreview *ensure(QGraphicsView *view, ColorPalette *palette = nullptr,
                                            ClipColorAssignmentBackend *document = nullptr) {
        if (!view) return nullptr;
        if (!document) document = platformClipColorAssignmentBackend();
        auto found = dynamic_cast<TimelineClipColorPreview *>(view->findChild<QObject *>(
            "openGateTimelineClipColors", Qt::FindDirectChildrenOnly));
        if (!found) found = new TimelineClipColorPreview(view, view, palette, document);
        else {
            if (document && !found->document) {
                delete found->restore;
                found->document = document;
                found->restore = new TimelineClipColorRestore(view, found, [found] { found->reload(); });
                if (found->palette && !found->paletteLife.expired()) ClipColorDefinitionUpdates::attach(*found->palette, *document, view);
            }
            found->setPalette(palette);
        }
        return found;
    }
    bool persistent() const { return document && document->isActive(); }
    void requestReload() { if (restore) restore->request(); }
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
        if (persistent()) {
            if (!style.opaque()) return 0;
            const auto fallback = ClipColorFallback::nearest(style);
            const ClipColorAssignment assignment{style, fallback};
            return commitSelected(true, assignment.encode(), fallback, "Set Clip Color: " + style.name);
        }
        clearInactiveDocumentBindings();
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
            QString next;
            if (persistent()) {
                const auto value = document->read(clip.object);
                const auto record = value.available && value.present
                    ? ClipColorAssignment::decode(value.bytes) : std::optional<ClipColorAssignment>{};
                if (record && nativeMatches(value.nativeColor, record->fallback)) next = record->paint.id;
            } else if (!documentBindingsActive) next = bindings ? bindings->presetId(clip.object) : QString();
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
        if (persistent()) { reload(); return; } // Native observer must never append a second history event.
        clearInactiveDocumentBindings();
        QSet<QGraphicsObject *> selected;
        for (const auto &clip : TimelineClipSelection::selected(view))
            selected.insert(clip.object);
        if (bindings && !selected.isEmpty())
            bindings->clear(selected);
    }
    int assignNative(const QString &name) {
        return persistent() ? commitSelected(false, {}, name, name.isEmpty() ? "Clear Clip Color" : "Set Clip Color: " + name) : 0;
    }
    void reload() {
        if (!bindings) return;
        if (!persistent()) { clearInactiveDocumentBindings(); return; }
        documentBindingsActive = true;
        TimelineClipColorBindings::RepaintBatch repaint(bindings);
        bindings->clear();
        for (const auto &clip : allClips()) {
            const auto value = document->read(clip.object);
            if (!value.available || !value.present) continue;
            const auto record = ClipColorAssignment::decode(value.bytes);
            if (record && record->paint.opaque() && nativeMatches(value.nativeColor, record->fallback))
                bindings->apply(clip.surface, clip.object, record->paint);
        }
    }

  private:
    QPointer<QGraphicsView> view;
    QPointer<TimelineClipColorBindings> bindings;
    QPointer<ClipColorAssignmentBackend> document;
    TimelineClipColorRestore *restore = nullptr;
    bool documentBindingsActive = false;
    ColorPalette *palette = nullptr;
    std::weak_ptr<const int> paletteLife;
    void clearInactiveDocumentBindings() {
        if (!documentBindingsActive) return;
        documentBindingsActive = false;
        if (bindings) bindings->clear();
    }
    static bool nativeMatches(QString native, QString fallback) {
        if (native.startsWith("ClipColor")) native.remove(0, 9);
        return native == fallback;
    }
    QVector<TimelineClipSelection::Clip> allClips() const {
        QVector<TimelineClipSelection::Clip> result;
        QSet<QGraphicsObject *> seen;
        for (auto surface : TimelineClipSelection::views(view))
            if (surface->scene())
                for (auto item : surface->scene()->items())
                    if (auto clip = TimelineClipSelection::container(item))
                        if (!seen.contains(clip)) { seen.insert(clip); result.append({surface, clip}); }
        return result;
    }
    int commitSelected(bool present, const QByteArray &bytes, const QString &native, const QString &label) {
        const auto selected = TimelineClipSelection::selected(view);
        QVector<ClipColorAssignmentBackend::Edit> edits;
        for (const auto &clip : selected) {
            const auto before = document->read(clip.object);
            if (!before.available) return 0; // No partial selected-clip operation or temporary fallback.
            edits.append({clip.object, before, present, bytes, native});
        }
        const bool committed = !edits.isEmpty() && document->commit(edits, label);
        logEvent("clip-color.assignment.requested", {{"clips", edits.size()}, {"committed", committed},
                 {"temporary", false}, {"project_writes", committed}});
        if (committed) reload();
        return committed ? edits.size() : 0;
    }
    void setPalette(ColorPalette *next) {
        if (!next || (palette == next && !paletteLife.expired())) return;
        palette = next;
        paletteLife = next->lifetime();
        if (document) ClipColorDefinitionUpdates::attach(*next, *document, view);
        next->subscribe(this, [this, next] {
            if (palette != next || !bindings) return;
            if (!persistent()) {
                clearInactiveDocumentBindings();
                bindings->refreshDefinitions(next->state().presets);
            }
            else reload();
        });
    }
};
} // namespace Unbound
