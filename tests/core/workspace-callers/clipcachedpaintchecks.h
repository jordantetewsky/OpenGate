// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/timelineclipcolorpreview.h"

namespace OpenGateTest {
template<class Check> void clipCachedPaintChecks(Check check) {
    // Repainting with the same fill must leave native alpha composition intact.
    for(bool childOpacity:{false,true})for(double factor:{.7,1.,1.47,2.13,3.}) {
        TimelineViewportFixture fixture;fixture.view->clip->hide();
        auto container=new UiTiClipContainer;fixture.view->tracksScene->addItem(container);container->setSelected(true);
        container->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        if(childOpacity) {
            container->setFlag(QGraphicsItem::ItemHasNoContents);
            auto child=new UiTiClipContainer;child->setParentItem(container);child->setFlag(QGraphicsItem::ItemIsSelectable,false);
            child->setCacheMode(QGraphicsItem::DeviceCoordinateCache);child->setOpacity(.5);
        } else container->setOpacity(.5);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.}});scale.apply(&fixture.timeline);
        TimelineViewportFixture::settle();auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto frame=[&]{TimelineViewportFixture::settle();return content->presentation()->grab().toImage();};
        const auto before=frame();Unbound::TimelineClipColorPreview preview(fixture.view,content);
        preview.applySelected(container->fill);const auto after=frame();
        check(after==before,"same-color preview preserves native cached opacity and all painted pixels");
        preview.clear();check(container->cacheMode()==QGraphicsItem::DeviceCoordinateCache,"opacity fallback restores the native cache policy");
    }
    for(auto cache:{QGraphicsItem::NoCache,QGraphicsItem::DeviceCoordinateCache,QGraphicsItem::ItemCoordinateCache})
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        TimelineViewportFixture fixture;fixture.view->clip->hide();
        auto container=new UiTiClipContainer;container->setFlag(QGraphicsItem::ItemHasNoContents);
        fixture.view->tracksScene->addItem(container);container->setSelected(true);
        auto child=new UiTiClipContainer;child->setParentItem(container);child->setFlag(QGraphicsItem::ItemIsSelectable,false);child->setCacheMode(cache);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});scale.apply(&fixture.timeline);
        TimelineViewportFixture::settle();auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto frame=[&]{TimelineViewportFixture::settle();return content->presentation()->grab().toImage();};
        auto pixel=[&](const QImage &image,int x,int y){return image.pixelColor((QPointF((content->headerWidth()+x)*factor,y*factor)*image.devicePixelRatioF()).toPoint());};
        const auto before=frame();Unbound::TimelineClipColorPreview preview(fixture.view,content);
        check(preview.applySelected(Qt::red)==1,"selected no-contents container reaches its original child paint source");
        const auto after=frame();
        if(cache==QGraphicsItem::ItemCoordinateCache) {
            check(!container->graphicsEffect() && pixel(after,200,60)==child->fill && child->cacheMode()==cache,
                  "item-coordinate caches remain untouched and restore native paint when no fill is exposed");
        } else {
            check(pixel(after,200,60)==QColor(Qt::red),"device-cached native child exposes its solid fill at every scale");
            auto at=[&](int x,int y){return (QPointF((content->headerWidth()+x)*factor,y*factor)*after.devicePixelRatioF()).toPoint();};
            const QRect ink(at(8,80),at(168,125));
            check(after.copy(ink)==before.copy(ink),"cached-child text and waveform pixels retain the native cache's raster phase");
            check(pixel(after,20,55)==pixel(before,20,55) && child->fill==QColor(30,130,180),
                  "cache bypass preserves thumbnail pixels and native model color");
            preview.clear();check(child->cacheMode()==cache && pixel(frame(),200,60)==child->fill,
                  "clear restores the original native cache mode and pixels");
            preview.applySelected(Qt::red);frame();delete container;preview.clear();
        }
    }
}
}
