// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <QRectF>
#include <QVector>
namespace Unbound {
// Match large solid body primitives to unambiguous clip bounds in viewport space.
struct ClipFillRegion {QRectF rect;QColor color;QRectF visible{};};
inline QColor clipRegionReplacement(const QRectF &rect,QColor source,const QVector<ClipFillRegion> &regions) {
    if(rect.isEmpty() || source.alpha()==0 || source.hsvSaturationF()<=.25 || source.valueF()<=.2)return {};
    QColor result;
    for(const auto &region:regions) {
        const auto visible=region.visible.isEmpty()?region.rect:region.visible;
        if(region.rect.isEmpty() || !region.rect.adjusted(-1,-1,1,1).contains(rect) ||
           rect.width()*rect.height()<visible.width()*visible.height()*.35)continue;
        if(result.isValid() && result!=region.color)return {}; // Ambiguous overlap stays native.
        result=region.color;
    }
    return result;
}
}
