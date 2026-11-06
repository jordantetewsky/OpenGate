// SPDX-License-Identifier: MIT
#pragma once
#include "clipfillregionmap.h"
#include "clipninepatch.h"
#include "clipopaquemask.h"
#include "clipskinpalette.h"
#include <QHash>
#include <QCache>
#include <QPixmap>
#include <QSet>
#include <functional>
namespace Unbound {
// Bounded cache of verified native nine-piece skins and their recolored variants.
// Clip ownership, input, native commands and persistent colors live elsewhere.
class ClipSkinPaint {
  public:
    using RejectedSkin = std::function<void(QRectF, QColor, QSize, qint64)>;
    void begin(QVector<ClipFillRegion> current, RejectedSkin rejected = {}) {
        regions = std::move(current);
        rejectedSkin = std::move(rejected);
        pending.clear();
        bodyColors.fill({}, regions.size());
        discovered = false;
    }
    bool learned() const {
        return discovered;
    }
    QColor mapBodyFill(QRectF target, QColor source) const {
        const int selected = regionIndex(target);
        if (selected < 0 || !source.isValid() || source.alpha() != 255 ||
            bodyColors.value(selected) != source)
            return {};
        return regions[selected].color;
    }
    QPainterPath opaqueMask(const QPixmap &native) {
        auto found = skins.find(native.cacheKey());
        if (found == skins.end())
            return {};
        auto &skin = found.value();
        if (!skin.maskReady) {
            skin.mask = clipOpaqueMask(skin.original);
            skin.maskReady = true;
        }
        return skin.mask;
    }
    QPixmap map(QRectF target, const QPixmap &native, QRectF source) {
        if (native.width() > 512 || native.height() > 512)
            return {};
        const int selected = regionIndex(target);
        if (selected < 0)
            return {};
        const auto key = native.cacheKey();
        const auto color = regions[selected].color;
        if (!color.isValid())
            return {};
        if (!skins.contains(key)) {
            auto &tiles = pending[{key, selected}];
            if (tiles.size() >= 9)
                return {};
            tiles.append({target, source});
            if (!clipNinePatch(tiles, regions[selected].rect, native.size()))
                return {};
            if (const auto center = invalidSkins.object(key)) {
                reportRejected(selected, native, *center);
                return {};
            }
            const auto image = native.toImage();
            if (image.isNull())
                return {};
            auto recolored = clipSkinPalette(image, color);
            if (recolored.isNull()) {
                const auto center = image.pixelColor(image.width() / 2, image.height() / 2);
                invalidSkins.insert(key, new QColor(center));
                reportRejected(selected, native, center);
                return {};
            }
            if (skins.size() >= 8)
                skins.clear();
            Skin skin;
            skin.original = image;
            skin.bodyColor = image.pixelColor(image.width() / 2, image.height() / 2);
            skin.colors.insert(color.rgba(), QPixmap::fromImage(recolored));
            skins.insert(key, std::move(skin));
            discovered = true;
        }
        auto &skin = skins[key];
        bodyColors[selected] = skin.bodyColor;
        if (!skin.colors.contains(color.rgba())) {
            auto image = clipSkinPalette(skin.original, color);
            if (image.isNull())
                return {};
            if (skin.colors.size() >= 4)
                skin.colors.clear();
            skin.colors.insert(color.rgba(), QPixmap::fromImage(image));
        }
        return skin.colors.value(color.rgba());
    }

  private:
    void reportRejected(int selected, const QPixmap &native, QColor center) {
        const auto key = native.cacheKey();
        if (rejectedSkin && rejectedKeys.size() < 16 && !rejectedKeys.contains(key)) {
            rejectedKeys.insert(key);
            rejectedSkin(regions[selected].rect, center, native.size(), key);
        }
    }
    int regionIndex(QRectF target) const {
        int selected = -1;
        for (int i = 0; i < regions.size(); ++i)
            if (regions[i].rect.contains(target.center()) &&
                regions[i].rect.adjusted(-3, -3, 3, 3).contains(target)) {
                if (selected >= 0)
                    return -1;
                selected = i;
            }
        return selected;
    }
    struct Skin {
        QImage original;
        QColor bodyColor;
        QHash<QRgb, QPixmap> colors;
        QPainterPath mask;
        bool maskReady = false;
    };
    QHash<qint64, Skin> skins;
    // Rejections depend only on unchanged source pixels once target validity
    // and this capture's complete nine-piece ownership have been established.
    QCache<qint64, QColor> invalidSkins{16};
    QHash<QPair<qint64, int>, QVector<ClipSkinTile>> pending;
    QVector<ClipFillRegion> regions;
    // A cached skin can be reused by many clips. Palette ownership is established
    // again during each capture, so a replaced scene/binding cannot inherit it.
    QVector<QColor> bodyColors;
    RejectedSkin rejectedSkin;
    QSet<qint64> rejectedKeys;
    bool discovered = false;
};
} // namespace Unbound
