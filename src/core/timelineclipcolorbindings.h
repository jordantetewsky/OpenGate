// SPDX-License-Identifier: MIT
#pragma once
#include "clipfillregionmap.h"
#include "cliplabelregions.h"
#include "colordisplaysettings.h"
#include "nativecommandactivity.h"
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QHash>
#include <QPointer>
#include <QSet>
namespace Unbound {
// One native root view owns weak clip/color bindings and their scene geometry.
// No painter, project writer, command handler or second scene is owned here.
class TimelineClipColorBindings : public QObject {
  public:
    // Coalesce display damage only. This is not a project/Undo transaction.
    class RepaintBatch {
      public:
        explicit RepaintBatch(TimelineClipColorBindings *owner) : owner(owner) {
            if (owner)
                ++owner->batchDepth;
        }
        RepaintBatch(const RepaintBatch &) = delete;
        RepaintBatch &operator=(const RepaintBatch &) = delete;
        ~RepaintBatch() {
            if (owner)
                owner->finishBatch();
        }

      private:
        QPointer<TimelineClipColorBindings> owner;
    };
    ~TimelineClipColorBindings() override {
        if (auto activity = NativeCommandActivity::existing())
            activity->unsubscribe(activityToken);
    }
    static TimelineClipColorBindings *find(QGraphicsView *root) {
        return root ? dynamic_cast<TimelineClipColorBindings *>(root->findChild<QObject *>(
                          QStringLiteral("openGateClipColorBindings"), Qt::FindDirectChildrenOnly))
                    : nullptr;
    }
    static TimelineClipColorBindings *ensure(QGraphicsView *root) {
        if (auto existing = find(root))
            return existing;
        return new TimelineClipColorBindings(root);
    }
    bool apply(QGraphicsView *surface, QGraphicsObject *clip, QColor color) {
        if (!color.isValid() || !surface || !clip || clip->scene() != surface->scene() ||
            clip->graphicsEffect() || surface->graphicsEffect() ||
            surface->viewport()->graphicsEffect())
            return false;
        if (!observed.contains(clip)) {
            observed.insert(clip);
            connect(clip, &QObject::destroyed, this, [this, clip] {
                observed.remove(clip);
                if (entries.remove(clip))
                    changed();
            });
        }
        auto &entry = entries[clip];
        entry.clip = clip;
        entry.surface = surface;
        entry.color = color;
        entry.style.clear();
        changed();
        return true;
    }
    bool apply(QGraphicsView *surface, QGraphicsObject *clip, const ColorPreset &style) {
        return apply(surface, clip, QSharedPointer<const ColorPreset>::create(style));
    }
    bool apply(QGraphicsView *surface, QGraphicsObject *clip,
               QSharedPointer<const ColorPreset> style) {
        if (!style || !style->opaque())
            return false;
        if (!style->color.isValid() || !apply(surface, clip, style->color))
            return false;
        if (!display) {
            display = ColorDisplaySettings::shared();
            if (display)
                display->subscribe(this, [this](bool) { changed(); });
        }
        auto &entry = entries[clip];
        entry.style = style;
        entry.labelSize = {};
        if (!activityToken)
            if (auto activity = NativeCommandActivity::shared())
                activityToken =
                    activity->subscribe(this, [this](const QString &) { ++labelEpoch; });
        return true;
    }
    QString presetId(QGraphicsObject *clip) const {
        const auto entry = entries.constFind(clip);
        if (entry != entries.cend() && entry->style)
            return entry->style->id;
        return {};
    }
    void clear(const QSet<QGraphicsObject *> &selected = {}) {
        const auto before = entries.size();
        if (selected.isEmpty())
            entries.clear();
        else
            for (auto clip : selected)
                entries.remove(clip);
        if (before != entries.size())
            changed();
    }
    QVector<ClipFillRegion> regions() const {
        QVector<ClipFillRegion> result;
        QHash<QGraphicsView *, QRectF> exposures;
        for (auto &entry : entries)
            if (entry.clip && entry.surface && entry.clip->isVisible() &&
                entry.clip->scene() == entry.surface->scene()) {
                const auto full = entry.clip->sceneBoundingRect();
                auto exposure = exposures.find(entry.surface);
                if (exposure == exposures.end())
                    exposure = exposures.insert(
                        entry.surface, entry.surface->mapToScene(entry.surface->viewport()->rect())
                                           .boundingRect());
                const auto exposed = full.intersected(exposure.value());
                if (exposed.isEmpty())
                    continue;
                const auto bounds = inRootScene(entry, full), visible = inRootScene(entry, exposed);
                if (!bounds.isEmpty() && !visible.isEmpty()) {
                    ClipFillRegion region{bounds, entry.color, visible, entry.style};
                    if (entry.style) {
                        if (entry.labelSize != full.size() || entry.labelEpoch != labelEpoch) {
                            entry.labels = clipLabelRegions(entry.clip);
                            entry.labelSize = full.size();
                            entry.labelEpoch = labelEpoch;
                        }
                        for (auto label : entry.labels)
                            region.labels.append(
                                inRootScene(entry, label.translated(full.topLeft())));
                    }
                    result.append(region);
                }
            }
        return result;
    }
    quint64 revision() const {
        return version;
    }
    bool texturesEnabled() const {
        return !display || display->texturesEnabled();
    }

  private:
    struct Entry {
        QPointer<QGraphicsObject> clip;
        QPointer<QGraphicsView> surface;
        QColor color;
        QSharedPointer<const ColorPreset> style{};
        QSizeF labelSize{};
        QVector<QRectF> labels{};
        quint64 labelEpoch = 0;
    };
    QPointer<QGraphicsView> root;
    QPointer<ColorDisplaySettings> display;
    mutable QHash<QGraphicsObject *, Entry> entries;
    QSet<QGraphicsObject *> observed;
    quint64 version = 0, activityToken = 0, labelEpoch = 1;
    int batchDepth = 0;
    bool pendingDamage = false;
    explicit TimelineClipColorBindings(QGraphicsView *root) : QObject(root), root(root) {
        setObjectName(QStringLiteral("openGateClipColorBindings"));
    }
    void changed() {
        if (batchDepth) {
            pendingDamage = true;
            return;
        }
        ++version;
        if (root) {
            if (root->scene())
                root->scene()->invalidate();
            if (root->viewport())
                root->viewport()->update();
        }
    }
    void finishBatch() {
        if (--batchDepth == 0 && pendingDamage) {
            pendingDamage = false;
            changed();
        }
    }
    QRectF inRootScene(const Entry &entry, QRectF bounds) const {
        if (!root)
            return {};
        auto surface = entry.surface.data();
        for (int depth = 0; surface && depth < 16; ++depth) {
            if (surface->scene() == root->scene())
                return bounds;
            bounds = surface->viewportTransform().mapRect(bounds);
            QWidget *host = surface;
            while (host && !host->graphicsProxyWidget())
                host = host->parentWidget();
            if (!host)
                return {};
            auto proxy = host->graphicsProxyWidget();
            bounds.translate(surface->viewport()->mapTo(host, QPoint()));
            bounds = proxy->mapRectToScene(bounds);
            if (proxy->scene() == root->scene())
                return bounds;
            const auto parents =
                proxy->scene() ? proxy->scene()->views() : QList<QGraphicsView *>();
            if (parents.size() != 1)
                return {};
            surface = parents.front();
        }
        return {};
    }
};
} // namespace Unbound
