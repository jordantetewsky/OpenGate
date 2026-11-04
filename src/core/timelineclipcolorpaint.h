// SPDX-License-Identifier: MIT
#pragma once
#include "clipfillpaintdevice.h"
#include "clipskinpaint.h"
#include "clipstylepaint.h"
#include "diagnostics.h"
#include "timelineclipcolorbindings.h"
namespace Unbound {
// Scope one native capture's rectangular fills using the existing root bindings.
// The renderer retains pixels/damage; this adapter owns only report deduplication.
class TimelineClipColorPaint {
  public:
    void render(QGraphicsView *view, QPainter *target,
                const std::function<void(QPainter *)> &native) {
        auto bindings = TimelineClipColorBindings::find(view);
        const auto regions = bindings ? bindings->regions() : QVector<ClipFillRegion>();
        bool invertible = false;
        const auto inverse = target->deviceTransform().inverted(&invertible);
        if (regions.isEmpty() || !invertible) {
            native(target);
            return;
        }
        skins.begin(regions);
        const bool styled = std::any_of(regions.cbegin(), regions.cend(),
                                        [](const auto &r) { return bool(r.style); });
        ClipFillPaintDevice::PaintHooks hooks;
        if (styled) {
            styles.begin(regions, inverse);
            hooks.brush = [this](const QRectF &r, QColor c, const QTransform &device) {
                return styles.brush(r, c, device);
            };
            hooks.afterBody = [this](QPainter *p, const QRectF &r, const QTransform &device) {
                styles.afterBody(p, r, device);
            };
            hooks.pixmap = [this](QPainter *p, const QRectF &r, const QPixmap &image,
                                  const QRectF &s, const QTransform &device) {
                return styles.pixmap(p, r, image, s, device);
            };
            hooks.text = [this](QPainter *p, const QPointF &position, const QTextItem &item,
                                const QTransform &device) {
                return styles.text(p, position, item, device);
            };
        }
        const auto result = ClipFillPaintDevice::mapRectangularFills(
            target,
            [this, regions, inverse, styled](const QRectF &r, QColor source,
                                             const QTransform &device) {
                const auto root = (device * inverse).mapRect(r);
                return styled && styles.owns(root) ? QColor()
                                                   : clipRegionReplacement(root, source, regions);
            },
            native, {},
            [this, inverse, styled](const QRectF &r, const QPixmap &pixmap, const QRectF &source,
                                    const QTransform &device) {
                const auto root = (device * inverse).mapRect(r);
                return styled && styles.owns(root) ? QPixmap() : skins.map(root, pixmap, source);
            },
            std::move(hooks));
        if (skins.learned()) {
            QPointer<QGraphicsView> alive = view;
            QTimer::singleShot(0, view, [alive] {
                if (alive) {
                    alive->scene()->invalidate();
                    alive->viewport()->update();
                }
            });
        }
        const bool first = reported != bindings || revision != bindings->revision();
        if (first)
            reportedRaster = false;
        if (first || (!reportedRaster && result.changedPixmaps > 0)) {
            reported = bindings;
            revision = bindings->revision();
            reportedRaster = result.changedPixmaps > 0;
            logEvent("clip-color.viewport.paint", {{"changed_fill_commands", result.changed},
                                                   {"rectangles", result.trace.rects},
                                                   {"pixmaps", result.trace.pixmaps},
                                                   {"changed_skin_pixmaps", result.changedPixmaps},
                                                   {"regions", regions.size()},
                                                   {"project_writes", false}});
        }
    }

  private:
    QPointer<TimelineClipColorBindings> reported;
    quint64 revision = 0;
    bool reportedRaster = false;
    ClipSkinPaint skins;
    ClipStylePaint styles{skins};
};
} // namespace Unbound
