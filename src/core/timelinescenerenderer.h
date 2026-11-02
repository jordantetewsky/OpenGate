// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QImage>
#include <QPainter>
#include <QPaintEngine>
#include <QPointer>
#include <QRegion>
#include <QHash>
#include <QVector>
#include <QtMath>
#include <algorithm>
#include <cstring>

namespace Unbound {
// Reuse presentation pixels and section raster devices. Each paint keeps the
// native 1:1 world coordinates and the same physical pixel origin at every scale.
// Scene/model ownership stays native; exposure flags are reversible leases.
class TimelineSceneRenderer {
public:
    TimelineSceneRenderer()=default;
    TimelineSceneRenderer(const TimelineSceneRenderer &)=delete;
    TimelineSceneRenderer &operator=(const TimelineSceneRenderer &)=delete;
    ~TimelineSceneRenderer() { reset(); }
    void reset() {
        for(const auto &entry:exposures)if(entry.proxy)
            entry.proxy->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption,entry.extended);
        exposures.clear();scene=nullptr;bands.clear();canvas=QSize();damage=QRegion();full=true;
    }
    void releasePixels() {bands.clear();invalidateAll();}
    qint64 rasterBytes() const {qint64 total=0;for(const auto &band:bands)total+=band.image.sizeInBytes();return total;}
    quint64 revision() const {return paintRevision;}
    bool pendingDamage() const {return full || !damage.isEmpty();}
    quint64 sampledFingerprint() const {
        quint64 hash=14695981039346656037ULL;
        for(const auto &band:bands)for(int y=0;y<8;++y)for(int x=0;x<16;++x) {
            const auto pixel=band.image.pixel(qMin(band.image.width()-1,(2*x+1)*band.image.width()/32),
                                              qMin(band.image.height()-1,(2*y+1)*band.image.height()/16));
            hash^=pixel;hash*=1099511628211ULL;
        }
        return hash;
    }
    QColor backgroundColor(const QRectF &area,QColor fallback) const {
        QHash<QRgb,int> counts;int most=0;QRgb selected=fallback.rgba();
        for(int y=0;y<8;++y)for(int x=0;x<16;++x) {
            const QPointF at(area.left()+area.width()*(x+.5)/16.,area.top()+area.height()*(y+.5)/8.);
            for(const auto &band:bands)if(band.rect.contains(at)) {
                const QPoint pixel(qFloor(at.x()*backing),qFloor(at.y()*backing-band.pixelTop));
                if(!band.image.rect().contains(pixel))continue;
                const auto color=band.image.pixel(pixel);if(qAlpha(color)!=255)continue;
                const int count=++counts[color];if(count>most){most=count;selected=color;}break;
            }
        }
        return QColor::fromRgba(selected);
    }
    void invalidateAll() { full=true;damage=QRegion(); }
    static QRectF damageBounds(const QRectF &area) {return area.adjusted(-4,-4,4,4);}
    void invalidate(const QRectF &area) {
        if(full || bands.isEmpty())return;
        if(!area.isValid()) {invalidateAll();return;}
        // The outer proxy and nested native view each expand antialias damage.
        const auto logical=damageBounds(area).translated(-source.topLeft());
        damage+=QRectF(logical.topLeft()*backing,logical.size()*backing).toAlignedRect().intersected(QRect(QPoint(),canvas));
        if(damage.rectCount()>16)damage=QRegion(damage.boundingRect());
    }
    bool render(QGraphicsScene *next,const QRectF &rect,double scale,const QVector<double> &sections) {
        if(scene!=next) {reset();scene=next;}
        if(!scene || !valid(rect,scale)) {releasePixels();return false;}
        if(source!=rect || backing!=scale || tops!=sections || bands.isEmpty()) {
            source=rect;backing=scale;tops=sections;
            canvas=QSize(qCeil(source.width()*backing),qCeil(source.height()*backing));
            if(!configureBands()) {releasePixels();return false;}
            invalidateAll();
        }
        exposeProxies();
        auto pending=full?QRegion(QRect(QPoint(),canvas)):damage;
        if(pending.isEmpty())return true;
        if(pending.rectCount()>16)pending=QRegion(pending.boundingRect());
        damage=QRegion();full=false;
        for(auto &band:bands) {
            for(const auto &pixels:pending) {
                const auto hit=logical(pixels).intersected(band.rect);
                if(hit.isEmpty())continue;
                // Full-sized section devices keep QWidget/native font hinting
                // on the same pixel phase. Only damaged pixels are cleared/copied.
                {
                    const auto clipped=pixels.translated(0,-band.pixelTop).intersected(band.image.rect());
                    clearPixels(band.image,QRegion(clipped));
                    auto engine=band.image.paintEngine();const auto previous=engine->systemClip();
                    engine->setSystemClip(QRegion(clipped));
                    {
                    QPainter native(&band.image);
                    native.translate(-source.x(),-source.y()-band.pixelTop/backing);
                    const auto area=hit.adjusted(-2,-2,2,2)
                        .intersected(band.rect).translated(source.topLeft());
                    scene->render(&native,area,area,Qt::IgnoreAspectRatio);
                    }
                    engine->setSystemClip(previous);
                }
            }
        }
        ++paintRevision;return true;
    }
    void draw(QPainter &painter,const QRectF &from,const QRectF &to) const {
        if(from.isEmpty() || to.isEmpty())return;
        const double x=to.width()/from.width(),y=to.height()/from.height();
        for(const auto &band:bands) {
            const auto hit=from.intersected(band.rect);if(hit.isEmpty())continue;
            const auto at=to.topLeft()+QPointF((hit.left()-from.left())*x,(hit.top()-from.top())*y);
            const QRectF target(at,QSizeF(hit.width()*x,hit.height()*y));
            const QRectF pixels(hit.left()*backing,hit.top()*backing-band.pixelTop,hit.width()*backing,hit.height()*backing);
            painter.save();painter.setClipRect(target,Qt::IntersectClip);
            painter.drawImage(target,band.image,pixels);painter.restore();
        }
    }
private:
    struct Exposure {QPointer<QGraphicsProxyWidget> proxy;bool extended;};
    struct Band {QRectF rect;int pixelTop=0;QImage image;};
    static void clearPixels(QImage &target,const QRegion &region) {
        // Physical integer bounds avoid fractional-DPR fill rounding into a
        // neighboring cached pixel that is outside this repaint's copy region.
        auto bits=target.bits();const auto stride=target.bytesPerLine();
        for(const auto &rect:region) {
            const auto clipped=rect.intersected(target.rect());if(clipped.isEmpty())continue;
            for(int y=clipped.top();y<=clipped.bottom();++y)
                std::memset(bits+y*stride+clipped.left()*4,0,size_t(clipped.width())*4);
        }
    }
    static bool valid(const QRectF &rect,double scale) {
        if(rect.isEmpty() || !qIsFinite(scale) || scale<=0. || !qIsFinite(rect.x()) || !qIsFinite(rect.y()) ||
           !qIsFinite(rect.width()) || !qIsFinite(rect.height()))return false;
        const double width=rect.width()*scale,height=rect.height()*scale;
        return width<=16384 && height<=16384 && width*height<=64.*1024*1024;
    }
    QRectF logical(QRect pixels) const {return {QPointF(pixels.topLeft())/backing,QSizeF(pixels.size())/backing};}
    bool configureBands() {
        QVector<double> edges{0.,source.height()};
        for(auto top:tops)if(qIsFinite(top) && top>0. && top<source.height())edges.append(top);
        std::sort(edges.begin(),edges.end());edges.erase(std::unique(edges.begin(),edges.end()),edges.end());
        bands.resize(edges.size()-1);
        for(int i=1;i<edges.size();++i) {
            auto &band=bands[i-1];band.rect=QRectF(0,edges[i-1],source.width(),edges[i]-edges[i-1]);
            band.pixelTop=qFloor(edges[i-1]*backing);
            const QSize size(canvas.width(),qCeil(edges[i]*backing)-band.pixelTop);
            if(band.image.size()!=size)band.image=QImage(size,QImage::Format_ARGB32_Premultiplied);
            band.image.setDevicePixelRatio(backing);
            if(band.image.isNull())return false;
        }
        return true;
    }
    void exposeProxies() {
        for(int i=exposures.size()-1;i>=0;--i)if(!exposures[i].proxy)exposures.removeAt(i);
        for(auto item:scene->items())if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
            bool known=false;for(const auto &entry:exposures)if(entry.proxy==proxy){known=true;break;}
            if(known)continue;
            exposures.append({proxy,proxy->flags().testFlag(QGraphicsItem::ItemUsesExtendedStyleOption)});
            proxy->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption,true);
        }
    }
    QPointer<QGraphicsScene> scene;QSize canvas;QRectF source;double backing=0.;QVector<double> tops;
    QRegion damage;bool full=true;QVector<Exposure> exposures;QVector<Band> bands;
    quint64 paintRevision=0;
};
}
