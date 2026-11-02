// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QGraphicsObject>
#include <QScrollBar>
#include <QVector>
#include <QRegion>
#include <QtMath>

namespace Unbound {
// Native section/row geometry and one inverse presentation map. Header content
// can shrink independently; track row positions and heights always stay shared.
struct TimelineTrackHeaderLayout {
    struct Slice { QRectF source, target;bool background=false; };
    double nativeWidth=0.,width=0.,nominalWidth=0.,offset=0.,ratio=1.,scrollAway=0.;
    double minimumBodyWidth=32.,minimumHeight=0.,nativePan=0.,footerControlsWidth=0.;
    bool pinned=true;
    QRectF horizontalBar,footer;
    QSizeF sourceSize,targetSize;
    QVector<QRectF> headerRows;
    QVector<double> sectionTops;
    double firstTrackTop=0.;

    static bool isType(QGraphicsItem *item,const char *type) {
        auto object=item?item->toGraphicsObject():nullptr;return object && object->inherits(type);
    }
    static TimelineTrackHeaderLayout discover(QGraphicsView *view) {
        TimelineTrackHeaderLayout result;if(!view || !view->scene())return result;
        result.sourceSize=view->viewport()->size();result.firstTrackTop=view->viewport()->height();
        double trackMinimum=0.,separators=0.;
        for(auto item:view->scene()->items()) {
            if(!item->isVisible())continue;
            if(isType(item,"UiTiSequenceSceneSplitterHandle"))separators+=item->boundingRect().height();
            if(isType(item,"UiTiScrollBarItem")) {
                const auto bounds=view->viewportTransform().mapRect(item->sceneBoundingRect());
                if(bounds.width()>bounds.height()*4 && bounds.bottom()>=view->viewport()->height()-24)result.horizontalBar=bounds;
            }
            auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
            auto nested=proxy?qobject_cast<QGraphicsView *>(proxy->widget()):nullptr;
            if(!nested)continue;
            const auto bounds=view->viewportTransform().mapRect(proxy->sceneBoundingRect());
            if(nested->objectName().endsWith("TracksView") && proxy->size().height()>0.) {
                result.sectionTops.append(bounds.top());
                const auto minimum=proxy->effectiveSizeHint(Qt::MinimumSize);
                result.minimumBodyWidth=qMax(result.minimumBodyWidth,minimum.width());
                trackMinimum+=qMax(32.,minimum.height());result.firstTrackTop=qMin(result.firstTrackTop,bounds.top());
                result.nativePan=qMax(result.nativePan,qMax(0.,nested->mapToScene(nested->viewport()->rect()).boundingRect().left()-nested->sceneRect().left()));
            }
            if(!nested->scene() || !nested->objectName().endsWith("TrackControlsView") || qAbs(bounds.left())>2 || bounds.width()<40)continue;
            result.nativeWidth=qMax(result.nativeWidth,bounds.width());
            bool hasRows=false;
            for(auto child:nested->scene()->items())if(child->isVisible() && isType(child,"UiTiTrackControlItem")) {
                auto local=nested->viewportTransform().mapRect(child->sceneBoundingRect());local.translate(nested->viewport()->pos());
                const auto row=view->viewportTransform().mapRect(proxy->mapRectToScene(local)).intersected(bounds);
                if(!row.isEmpty()){result.headerRows.append(row);hasRows=true;}
            }
            if(!hasRows && !bounds.isEmpty())result.headerRows.append(bounds);
        }
        if(trackMinimum>0.)result.minimumHeight=result.firstTrackTop+trackMinimum+separators+result.horizontalBar.height();
        return result;
    }
    void configure(QSizeF target,double requestedWidth,double requestedOffset,bool pin,double footerHeight=0.,
                   double headerRatio=1.,double minimumFooter=0.,double footerControls=0.) {
        targetSize=target;ratio=qBound(.0001,headerRatio,1.);pinned=pin;footerControlsWidth=footerControls;
        const auto height=qBound(0.,footerHeight>0.?footerHeight:horizontalBar.height(),target.height());
        footer=QRectF(0,target.height()-height,target.width(),height);
        const double maximum=qMax(0.,qMin(nativeWidth*ratio,target.width()-minimumBodyWidth));
        nominalWidth=qBound(qMin(qMax(40.*ratio,minimumFooter),maximum),
            (requestedWidth>0.?requestedWidth:nativeWidth)*ratio,maximum);
        scrollAway=pinned?0.:qMin(nominalWidth,nativePan*ratio);width=nominalWidth-scrollAway;
        offset=qBound(0.,requestedOffset,maximumOffset());
    }
    double maximumOffset() const {return qMax(0.,nativeWidth-nominalWidth/ratio);}
    QPointF toNative(QPointF point) const {
        const auto parts=slices();
        for(auto it=parts.crbegin();it!=parts.crend();++it)if(it->target.contains(point) && !it->target.isEmpty()) {
            if(it->background)return {nativeWidth-1,it->source.top()+(point.y()-it->target.top())*it->source.height()/it->target.height()};
            return it->source.topLeft()+QPointF((point.x()-it->target.x())*it->source.width()/it->target.width(),
                                              (point.y()-it->target.y())*it->source.height()/it->target.height());
        }
        if(point.x()>=width)point.rx()+=nativeWidth-width;
        return point;
    }
    QRegion presentationDamage(const QRectF &native,double scale) const {
        QRegion result;
        for(const auto &slice:slices()) {
            const auto hit=native.intersected(slice.source);if(hit.isEmpty() || slice.source.isEmpty())continue;
            if(slice.background){result+=QRectF(slice.target.topLeft()*scale,slice.target.size()*scale).toAlignedRect();continue;}
            const double x=slice.target.width()/slice.source.width(),y=slice.target.height()/slice.source.height();
            const auto at=slice.target.topLeft()+QPointF((hit.left()-slice.source.left())*x,(hit.top()-slice.source.top())*y);
            result+=QRectF(at*scale,QSizeF(hit.width()*x*scale,hit.height()*y*scale)).toAlignedRect().adjusted(-2,-2,2,2);
        }
        return result;
    }
    QVector<Slice> slices() const {
        QVector<Slice> result;
        const double bottom=horizontalBar.isEmpty()?footer.top():qMin(footer.top(),horizontalBar.top());
        const QRectF header(0,0,width,bottom);
        if(width>0. && bottom>0.) {
            // Background color comes from the native row raster; no strip of
            // text/button pixels is stretched to fill the unused row height.
            result.append({QRectF(0,0,nativeWidth,bottom),header,true});
            auto row=[&](QRectF source,bool timecode=false) {
                if(source.isEmpty())return;
                QRectF target(0,source.top()+(timecode?source.height()*(1-ratio)*.5:0.),width,source.height()*ratio);
                const auto clipped=target.intersected(header);if(clipped.isEmpty())return;
                source.setX(offset+scrollAway/ratio);source.setWidth(width/ratio);
                source.setTop(source.top()+(clipped.top()-target.top())/ratio);
                source.setHeight(clipped.height()/ratio);result.append({source,clipped});
            };
            row(QRectF(0,0,nativeWidth,qMin(bottom,firstTrackTop)),true);
            for(const auto &native:headerRows) {
                const auto bounds=native.intersected(QRectF(0,0,nativeWidth,bottom));
                if(!bounds.isEmpty())result.append({bounds,QRectF(0,bounds.top(),width,bounds.height()),true});
                row(native);
            }
        }
        result.append({QRectF(nativeWidth,0,qMax(0.,targetSize.width()-width),bottom),
                       QRectF(width,0,qMax(0.,targetSize.width()-width),bottom)});
        if(!horizontalBar.isEmpty()) {
            const auto left=qMax(width,footerControlsWidth);
            result.append({horizontalBar,QRectF(left,footer.top(),qMax(0.,targetSize.width()-left),footer.height())});
        }
        return result;
    }
};
}
