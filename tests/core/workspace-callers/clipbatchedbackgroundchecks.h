// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/timelineclipcolorpreview.h"
namespace OpenGateTest {
template<class Check> void clipBatchedBackgroundChecks(Check check) {
    {
        QImage pixels(80,30,QImage::Format_ARGB32_Premultiplied);pixels.fill(Qt::transparent);QPainter painter(&pixels);
        int paints=0;const QVector<Unbound::ClipFillRegion> regions{{QRectF(0,0,30,20),Qt::red}};
        const auto result=Unbound::ClipFillPaintDevice::mapRectangularFills(&painter,
            [regions](const QRectF &r,QColor c,const QTransform &){return Unbound::clipRegionReplacement(r,c,regions);},
            [&paints](QPainter *p){++paints;p->setPen(Qt::NoPen);p->setBrush(QColor(30,130,180));
                const QRectF batch[]{QRectF(0,0,30,20),QRectF(40,0,30,20)};p->drawRects(batch,2);});
        painter.end();
        check(paints==1 && result.changed==1 && pixels.pixelColor(10,10)==QColor(Qt::red) && pixels.pixelColor(50,10)==QColor(30,130,180),
              "one native paint pass changes only the selected rectangle in a shared color batch");
    }
    for(auto cache:{QGraphicsItem::NoCache,QGraphicsItem::DeviceCoordinateCache,QGraphicsItem::ItemCoordinateCache})
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        if(qEnvironmentVariableIsSet("OPENGATE_TEST_ONE_SCALE") && factor!=1.)continue;
        TimelineViewportFixture fixture;fixture.view->clip->hide();
        auto tracks=dynamic_cast<BatchedClipSceneView *>(fixture.view->tracksProxy->widget());tracks->clipBackground=true;
        auto clip=new UiTiClipContainer;clip->setFlag(QGraphicsItem::ItemHasNoContents);clip->setSelected(true);
        fixture.view->tracksScene->addItem(clip);
        auto details=new UiTiClipContainer;details->paintBody=false;details->setFlag(QGraphicsItem::ItemIsSelectable,false);
        details->setParentItem(clip);details->setCacheMode(cache);
        auto other=new UiTiClipContainer;other->setFlag(QGraphicsItem::ItemHasNoContents);other->setPos(0,110);
        fixture.view->tracksScene->addItem(other);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});scale.apply(&fixture.timeline);
        TimelineViewportFixture::settle();auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto frame=[&]{tracks->viewport()->update();TimelineViewportFixture::settle();return content->presentation()->grab().toImage();};
        auto at=[&](QPointF point,const QImage &im){return (QPointF((content->headerWidth()+point.x())*factor,(40+point.y())*factor)*im.devicePixelRatioF()).toPoint();};
        const auto before=frame();Unbound::TimelineClipColorPreview preview(fixture.view,content);
        check(preview.applySelected(Qt::red)==1,"native selection binds to the view that batches clip bodies");
        const auto after=frame();
        check(after.pixelColor(at({200,20},after))==QColor(Qt::red),"batched body outside the clip subtree paints red at every content scale");
        check(after.pixelColor(at({200,130},after))==other->fill,"same-colored unselected clip in the same native batch retains its color");
        check(after.copy(QRect(at({8,40},after),at({168,85},after)))==before.copy(QRect(at({8,40},before),at({168,85},before))),
              "cached glyphs and waveform pixels are preserved when bodies paint in the view background");
        check(after.pixelColor(at({20,15},after))==before.pixelColor(at({20,15},before)) && details->cacheMode()==cache && !clip->graphicsEffect(),
              "thumbnail pixels and native item caches/effects remain untouched");
        clip->setSelected(false);other->setSelected(true);preview.clearSelected();
        check(frame().pixelColor(at({200,20},after))==QColor(Qt::red),"clearing another selected clip does not clear this clip's assignment");
        if(factor==1.) {
            tracks->clipBatchToExposure=true;clip->setWidth(4000);
            check(frame().pixelColor(at({200,20},after))==QColor(Qt::red),"a long clip still paints when the native batch contains only its exposed portion");
            clip->setWidth(240);tracks->clipBatchToExposure=false;
        }
        preview.clear();check(frame()==before && !tracks->viewport()->graphicsEffect(),"clear removes the color binding and restores exact native pixels without a viewport effect");
    }
}
}
