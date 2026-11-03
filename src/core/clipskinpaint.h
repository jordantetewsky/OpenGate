// SPDX-License-Identifier: MIT
#pragma once
#include <QPixmap>
#include <QHash>
#include "clipfillregionmap.h"
#include "clipninepatch.h"
#include "clipskinpalette.h"
namespace Unbound {
// Bounded cache of verified native nine-piece skins and their recolored variants.
// Clip ownership, input, native commands and persistent colors live elsewhere.
class ClipSkinPaint {
public:
    void begin(QVector<ClipFillRegion> current){regions=std::move(current);pending.clear();discovered=false;}
    bool learned() const{return discovered;}
    QPixmap map(QRectF target,const QPixmap &native,QRectF source) {
        if(native.width()>512 || native.height()>512)return {};
        int selected=-1;
        for(int i=0;i<regions.size();++i)if(regions[i].rect.contains(target.center()) && regions[i].rect.adjusted(-3,-3,3,3).contains(target)) {
            if(selected>=0)return {};selected=i;
        }
        if(selected<0)return {};
        const auto key=native.cacheKey();const auto color=regions[selected].color;
        if(!skins.contains(key)) {
            auto &tiles=pending[{key,selected}];if(tiles.size()>=9)return {};tiles.append({target,source});
            if(!clipNinePatch(tiles,regions[selected].rect,native.size()))return {};
            const auto image=native.toImage();auto recolored=clipSkinPalette(image,color);if(recolored.isNull())return {};
            if(skins.size()>=8)skins.clear();Skin skin;skin.original=image;skin.colors.insert(color.rgba(),QPixmap::fromImage(recolored));
            skins.insert(key,std::move(skin));discovered=true;
        }
        auto &skin=skins[key];
        if(!skin.colors.contains(color.rgba())) {
            auto image=clipSkinPalette(skin.original,color);if(image.isNull())return {};
            if(skin.colors.size()>=4)skin.colors.clear();skin.colors.insert(color.rgba(),QPixmap::fromImage(image));
        }
        return skin.colors.value(color.rgba());
    }
private:
    struct Skin {QImage original;QHash<QRgb,QPixmap> colors;};
    QHash<qint64,Skin> skins;QHash<QPair<qint64,int>,QVector<ClipSkinTile>> pending;
    QVector<ClipFillRegion> regions;bool discovered=false;
};
}
