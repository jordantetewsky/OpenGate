// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "diagnostics.h"
#include "timelineclipcolorbindings.h"
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
        for (auto surface : views())
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems())
                    if (container(item))
                        return true;
        return false;
    }
    int applySelected(QColor color) {
        if (!color.isValid())
            return 0;
        TimelineClipColorBindings::RepaintBatch repaint(bindings);
        int count = 0;
        QSet<QGraphicsObject *> seen;
        for (auto surface : views())
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems()) {
                    auto clip = container(item);
                    if (!clip || seen.contains(clip))
                        continue;
                    seen.insert(clip);
                    if (bindings)
                        count += bindings->apply(surface, clip, color);
                }
        logEvent("clip-color.preview.requested", {{"clips", count},
                                                  {"color", color.name()},
                                                  {"temporary", true},
                                                  {"project_writes", false}});
        return count;
    }
    int applySelected(const ColorPreset &style) {
        TimelineClipColorBindings::RepaintBatch repaint(bindings);
        int count = 0;
        QSet<QGraphicsObject *> seen;
        const auto definition = QSharedPointer<const ColorPreset>::create(style);
        for (auto surface : views())
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems()) {
                    auto clip = container(item);
                    if (!clip || seen.contains(clip))
                        continue;
                    seen.insert(clip);
                    if (bindings)
                        count += bindings->apply(surface, clip, definition);
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
        for (auto surface : views())
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems())
                    if (auto clip = container(item)) {
                        const auto next = bindings ? bindings->presetId(clip) : QString();
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
        for (auto surface : views())
            if (surface->scene())
                for (auto item : surface->scene()->selectedItems())
                    if (auto clip = container(item))
                        selected.insert(clip);
        if (bindings && !selected.isEmpty())
            bindings->clear(selected);
    }

  private:
    QPointer<QGraphicsView> view;
    QPointer<TimelineClipColorBindings> bindings;
    static QGraphicsObject *container(QGraphicsItem *item) {
        for (auto p = item; p; p = p->parentItem())
            if (auto object = p->toGraphicsObject())
                if (object->inherits("UiTiClipContainer"))
                    return object;
        return nullptr;
    }
    QList<QGraphicsView *> views() const {
        QList<QGraphicsView *> result, pending;
        if (view)
            pending.append(view);
        while (!pending.isEmpty() && result.size() < 32) {
            auto next = pending.takeFirst();
            if (!next || result.contains(next))
                continue;
            result.append(next);
            if (auto scene = next->scene())
                for (auto item : scene->items())
                    if (auto proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item))
                        if (auto nested = qobject_cast<QGraphicsView *>(proxy->widget()))
                            pending.append(nested);
        }
        return result;
    }
};
} // namespace Unbound
