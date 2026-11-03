// SPDX-License-Identifier: MIT
#pragma once
#include <QRectF>
#include <QVector>
#include <algorithm>
#include <cmath>
namespace Unbound {
struct ClipSkinTile {QRectF target,source;};
// Recognize a complete nine-piece rectangular skin, not a thumbnail-sized draw.
inline bool clipNinePatch(const QVector<ClipSkinTile> &tiles,QRectF clip,QSize sourceSize) {
    if(tiles.size()!=9)return false;
    auto grid=[](const QVector<QRectF> &rects,QRectF expected,double tolerance){
        QRectF total;double area=0.;QVector<double> xs,ys;
        for(const auto &r:rects){if(r.isEmpty())return false;total=total.united(r);area+=r.width()*r.height();xs<<r.left()<<r.right();ys<<r.top()<<r.bottom();}
        if(std::abs(total.left()-expected.left())>tolerance || std::abs(total.top()-expected.top())>tolerance ||
           std::abs(total.right()-expected.right())>tolerance || std::abs(total.bottom()-expected.bottom())>tolerance ||
           std::abs(area-total.width()*total.height())>.1)return false;
        auto count=[](QVector<double> values){std::sort(values.begin(),values.end());int n=0;double last=0.;
            for(double v:values)if(!n || std::abs(last-v)>.01){++n;last=v;}return n;};
        if(count(xs)!=4 || count(ys)!=4)return false;
        for(int i=0;i<rects.size();++i)for(int j=0;j<i;++j){const auto overlap=rects[i].intersected(rects[j]);if(overlap.width()*overlap.height()>.01)return false;}
        return true;
    };
    QVector<QRectF> targets,sources;for(const auto &tile:tiles){targets<<tile.target;sources<<tile.source;}
    return grid(targets,clip,3.) && grid(sources,QRectF(QPointF(),sourceSize),.1);
}
}
