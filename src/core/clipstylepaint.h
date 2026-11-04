// SPDX-License-Identifier: MIT
#pragma once
#include "clipskinpaint.h"
#include "colorpresetpaint.h"
#include <QCache>
#include <QHash>
namespace Unbound {
// Map verified native body/label primitives to shared preset paint. No native
// image recoloring, assignment ownership, input, history or project writes.
class ClipStylePaint {
  public:
    explicit ClipStylePaint(ClipSkinPaint &skins) : skins(skins) {}
    void begin(QVector<ClipFillRegion> current, QTransform deviceToRoot, bool textures = true) {
        regions = std::move(current);
        inverse = deviceToRoot;
        texturesEnabled = textures;
    }
    bool owns(QRectF rect) const {
        for (const auto &r : regions)
            if (r.style && r.rect.contains(rect.center()))
                return true;
        return false;
    }
    QBrush brush(const QRectF &rect, QColor source, const QTransform &device) {
        const auto basis = device * inverse;
        const auto root = basis.mapRect(rect);
        if (!clipBodyPrimitive(root, source) || source.alpha() != 255)
            return {};
        const auto region = find(root);
        if (!region || !clipBodyContains(*region, root) ||
            root.width() < region->rect.width() * .9 || root.height() < region->rect.height() * .9)
            return {};
        bool ok = false;
        const auto toLocal = basis.inverted(&ok);
        if (!ok)
            return {};
        return paint.brush(region->style, region->rect, toLocal);
    }
    void afterBody(QPainter *out, const QRectF &rect, const QTransform &device) {
        const auto basis = device * inverse;
        const auto root = basis.mapRect(rect);
        const auto region = find(root);
        bool ok = false;
        const auto toLocal = basis.inverted(&ok);
        if (!region || !ok)
            return;
        if (texturesEnabled)
            paint.texture(*out, rect, *region->style, region->rect, toLocal);
        labels(out, *region, toLocal);
    }
    bool pixmap(QPainter *out, const QRectF &rect, const QPixmap &native, const QRectF &source,
                const QTransform &device) {
        const auto basis = device * inverse;
        const auto root = basis.mapRect(rect);
        const auto region = find(root);
        if (!region)
            return false;
        bool ok = false;
        const auto toLocal = basis.inverted(&ok);
        if (!ok)
            return false;
        const auto replacement = skins.map(root, native, source);
        if (!replacement.isNull()) {
            out->drawPixmap(rect, replacement, source);
            if (source.isEmpty())
                return true;
            const auto mask = skins.opaqueMask(native);
            if (mask.isEmpty())
                return true;
            QTransform local;
            local.translate(rect.left(), rect.top());
            local.scale(rect.width() / source.width(), rect.height() / source.height());
            local.translate(-source.left(), -source.top());
            out->save();
            out->setClipRect(rect, Qt::IntersectClip);
            out->setClipPath(local.map(mask), Qt::IntersectClip);
            paint.fill(*out, rect, region->style, region->rect, texturesEnabled, toLocal);
            labels(out, *region, toLocal);
            out->restore();
            return true;
        }
        // Only an image contained by an identified native label can acquire a
        // backing. Unknown images/thumbnails/waveforms pass through untouched.
        for (const auto &label : region->labels)
            if (label.adjusted(-1, -1, 1, 1).contains(root) && native.width() <= 2048 &&
                native.height() <= 128) {
                const auto ink = labelInk(native);
                const auto style = colorBackingForInk(ink);
                out->save();
                out->fillRect(rect, style.scrim);
                out->drawPixmap(rect, native, source);
                out->restore();
                return true;
            }
        return false;
    }
    bool text(QPainter *out, const QPointF &position, const QTextItem &item,
              const QTransform &device) {
        const QRectF box(position.x(), position.y() - item.ascent(), item.width(),
                         item.ascent() + item.descent());
        const auto root = (device * inverse).mapRect(box);
        if (!find(root))
            return false;
        const auto style = colorBackingForInk(out->pen().color());
        out->save();
        out->fillRect(box.adjusted(-1, -1, 1, 1), style.scrim);
        out->drawTextItem(position, item);
        out->restore();
        return true;
    }

  private:
    ClipSkinPaint &skins;
    ColorPresetPaint paint;
    QVector<ClipFillRegion> regions;
    QTransform inverse;
    bool texturesEnabled = true;
    QCache<qint64, QColor> inks{128};
    const ClipFillRegion *find(QRectF rect) const {
        const ClipFillRegion *found = nullptr;
        for (const auto &r : regions)
            if (r.style && r.rect.contains(rect.center()) &&
                r.rect.adjusted(-3, -3, 3, 3).contains(rect)) {
                if (found)
                    return nullptr;
                found = &r;
            }
        return found;
    }
    void labels(QPainter *out, const ClipFillRegion &region, const QTransform &toLocal) {
        const auto style = colorBackingForInk(Qt::white);
        for (auto rect : region.labels)
            out->fillRect(toLocal.mapRect(rect.intersected(region.rect)), style.scrim);
    }
    QColor labelInk(const QPixmap &pixmap) {
        if (auto found = inks.object(pixmap.cacheKey()))
            return *found;
        const auto image = pixmap.toImage();
        QHash<QRgb, int> counts;
        QRgb best = qRgb(255, 255, 255);
        int maximum = 0;
        // Sample a bounded number of pixels once per cached native label. No
        // full-clip raster or per-frame image histogram is constructed.
        const int step =
            qMax(1, int(std::ceil(std::sqrt(double(image.width()) * image.height() / 4096.))));
        for (int y = 0; y < image.height(); y += step)
            for (int x = 0; x < image.width(); x += step) {
                const auto c = image.pixelColor(x, y);
                if (c.alpha() < 128)
                    continue;
                const auto rgb = c.rgb();
                const auto count = ++counts[rgb];
                if (count > maximum) {
                    maximum = count;
                    best = rgb;
                }
            }
        const auto ink = QColor::fromRgb(best);
        inks.insert(pixmap.cacheKey(), new QColor(ink));
        return ink;
    }
};
} // namespace Unbound
