// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QColor>
#include <algorithm>
#include <cmath>
namespace Unbound {
// Recolor a verified small skin's chromatic palette, preserving alpha and neutral
// shadows. The caller must establish native skin identity before calling this.
inline QImage clipSkinPalette(const QImage &input,QColor target) {
    if(input.isNull() || !target.isValid() || input.width()>512 || input.height()>512)return {};
    const auto base=input.pixelColor(input.width()/2,input.height()/2);
    if(base.alpha()==0 || base.hslSaturationF()<.1 || base.lightnessF()<.1)return {};
    auto result=input.convertToFormat(QImage::Format_ARGB32);int colored=0;
    for(int y=0;y<result.height();++y)for(int x=0;x<result.width();++x) {
        auto color=result.pixelColor(x,y);if(!color.alpha() || color.hslSaturationF()<.1)continue;
        auto distance=std::abs(color.hslHueF()-base.hslHueF());distance=std::min(distance,1.-distance);
        if(distance>.04)return {}; // Mixed hues are not a monochrome body skin.
        const auto saturation=std::clamp(color.hslSaturationF()/base.hslSaturationF()*target.hslSaturationF(),0.,1.);
        const auto lightness=std::clamp(target.lightnessF()+color.lightnessF()-base.lightnessF(),0.,1.);
        auto mapped=QColor::fromHslF(std::max(0.,target.hslHueF()),saturation,lightness,color.alphaF());
        if(color==base)mapped=QColor(target.red(),target.green(),target.blue(),color.alpha());
        result.setPixelColor(x,y,mapped);++colored;
    }
    if(!colored)return {};result.setDevicePixelRatio(input.devicePixelRatio());return result;
}
}
