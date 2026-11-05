// SPDX-License-Identifier: MIT
#pragma once
#include "clipfillregionmap.h"
#include "diagnostics.h"
#include <QJsonArray>
#include <QSet>
#include <QTransform>
#include <cstring>
#include <functional>

namespace Unbound {
// Opt-in, bounded metadata receipts for large native paint primitives within
// assigned clips. Never copies image pixels or changes a paint decision.
class ClipPaintTrace {
  public:
    using Record = std::function<void(const QJsonObject &)>;
    explicit ClipPaintTrace(
        bool enabled = qEnvironmentVariableIntValue("OPENGATE_COLOR_PAINT_TRACE") == 1,
        Record record =
            [](const QJsonObject &value) { logEvent("clip-color.native-primitive", value); })
        : enabled(enabled), record(std::move(record)) {}
    bool active() const {
        return enabled;
    }
    void begin(const QVector<ClipFillRegion> &current, QTransform deviceToRoot, quint64 revision) {
        if (!enabled)
            return;
        regions = current;
        inverse = deviceToRoot;
        if (revision != lastRevision) {
            lastRevision = revision;
            seen.clear();
        }
    }
    void observe(const char *kind, QRectF target, const QTransform &device, bool mapped,
                 QColor sourceColor = {}, QSize imageSize = {}, qint64 imageKey = 0,
                 QRectF imageSource = {}) {
        if (!enabled)
            return;
        observeRoot(kind, (device * inverse).mapRect(target), mapped, sourceColor, imageSize,
                    imageKey, imageSource);
    }
    void rejectedSkin(QRectF clip, QColor center, QSize size, qint64 key) {
        observeRoot("skin-palette-rejected", clip, false, center, size, key, {});
    }

  private:
    void observeRoot(const char *kind, QRectF root, bool mapped, QColor sourceColor,
                     QSize imageSize, qint64 imageKey, QRectF imageSource) {
        if (!enabled || seen.size() >= limit)
            return;
        const ClipFillRegion *match = nullptr;
        for (const auto &region : regions)
            if (region.style && region.rect.adjusted(-3, -3, 3, 3).contains(root)) {
                if (match)
                    return;
                match = &region;
            }
        if (!match || match->rect.isEmpty() || root.width() < 12 || root.height() < 12 ||
            root.width() < match->rect.width() * .15 || root.height() < match->rect.height() * .4)
            return;
        quint64 key = 14695981039346656037ULL;
        const auto mix = [&](quint64 value) { key = (key ^ value) * 1099511628211ULL; };
        const auto coordinate = [&](double value) {
            quint64 bits;
            static_assert(sizeof(bits) == sizeof(value));
            std::memcpy(&bits, &value, sizeof(bits));
            mix(bits);
        };
        for (const auto *c = kind; *c; ++c)
            mix(static_cast<unsigned char>(*c));
        mix(match->graphicsIdentity);
        mix(qHash(match->style->id));
        mix(mapped);
        mix(sourceColor.isValid() ? sourceColor.rgba() : 0);
        mix(imageKey);
        mix(imageSize.width());
        mix(imageSize.height());
        for (const auto &rect : {root, match->rect, imageSource})
            for (double value : {rect.x(), rect.y(), rect.width(), rect.height()})
                coordinate(value);
        if (seen.contains(key))
            return; // Repeated frame metadata does not allocate/serialize JSON.
        seen.insert(key);
        const auto box = [](QRectF r) { return QJsonArray{r.x(), r.y(), r.width(), r.height()}; };
        const auto &clip = match->rect;
        QJsonObject value{
            {"kind", kind},
            {"preset", match->style->id},
            {"graphics_item", QString::number(match->graphicsIdentity, 16)},
            {"graphics_identity_kind", "QGraphicsObject*"},
            {"binding_revision", double(lastRevision)},
            {"clip_scene_rect", box(clip)},
            {"primitive_scene_rect", box(root)},
            {"primitive_clip_ratio",
             box(QRectF((root.x() - clip.x()) / clip.width(), (root.y() - clip.y()) / clip.height(),
                        root.width() / clip.width(), root.height() / clip.height()))},
            {"mapped", mapped},
            {"read_only", true},
            {"pixel_content_captured", false},
            {"native_callsite_captured", false}};
        if (qstrcmp(kind, "skin-palette-rejected") == 0)
            value["complete_nine_patch_verified"] = true;
        if (sourceColor.isValid())
            value["source_color"] = sourceColor.name(QColor::HexArgb);
        if (!imageSize.isEmpty()) {
            value["image_size"] = QJsonArray{imageSize.width(), imageSize.height()};
            value["image_cache_key"] = QString::number(imageKey, 16);
            value["image_source_rect"] = box(imageSource);
        }
        if (record)
            record(value);
    }

  private:
    static constexpr int limit = 64;
    bool enabled;
    Record record;
    QVector<ClipFillRegion> regions;
    QTransform inverse;
    QSet<quint64> seen;
    quint64 lastRevision = 0;
};
} // namespace Unbound
