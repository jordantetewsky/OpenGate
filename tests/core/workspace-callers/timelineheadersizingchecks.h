// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QSpinBox>
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"

namespace OpenGateTest {
template<class Check> void timelineHeaderSizingChecks(Check check) {
    TimelineViewportFixture fixture;
    Unbound::PanelContentScale scale;
    scale.restoreState({{"interface_scale",3.},{"timeline_header_width",160.},{"timeline_header_scale",1.}});
    check(scale.apply(&fixture.timeline),"independent header scale enters through the saved production state owner");TimelineViewportFixture::settle();
    auto viewport=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    if(!viewport)return;auto layer=viewport->presentation();
    auto bar=layer->findChild<QScrollBar *>("openGateTimelineHeaderScroll");
    auto button=layer->findChild<QToolButton *>("openGateTimelineHeaderScale");
    const auto trackScene=fixture.view->tracksScene;const auto transform=fixture.view->transform();
    auto geometry=[&] {
        auto value=Unbound::TimelineTrackHeaderLayout::discover(fixture.view);
        value.configure(QSizeF(layer->width()/scale.value(),layer->height()/scale.value()),viewport->retainedHeaderWidth(),viewport->headerOffset(),
            viewport->headerPinned(),18./scale.value(),viewport->headerScale()/scale.value(),144./scale.value(),66./scale.value());return value;
    };
    auto locate=[&](QPointF source) {
        for(const auto &part:geometry().slices())if(!part.background && part.source.contains(source))
            return (part.target.topLeft()+QPointF((source.x()-part.source.x())*part.target.width()/part.source.width(),
                                                (source.y()-part.source.y())*part.target.height()/part.source.height()))*scale.value();
        return QPointF(-1,-1);
    };
    for(double body:{.7,1.,1.47,2.13,3.}) {
        scale.setValue(body);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        check(qAbs(viewport->headerScale()-qMin(1.,body))<.001 && qAbs(scale.snapshot()["timeline_header_scale"].toDouble()-1.)<.001,
              "compact track rows limit effective control size without overwriting the preferred header scale");
        bar->setValue(0);TimelineViewportFixture::settle();
        const auto map=geometry();
        for(const auto &row:map.headerRows)for(const auto &part:map.slices()) {
            if(part.background || part.source.top()<row.top()-.001 || part.source.bottom()>row.bottom()+.001)continue;
            check(part.target.top()>=row.top()-.001 && part.target.bottom()<=row.bottom()+.001,
                  "scaled header content remains within its matching native track row");
        }
        const int blue=fixture.view->blue->clicks,red=fixture.view->red->clicks;
        TimelineViewportFixture::click(layer,locate(QPointF(30,50)));TimelineViewportFixture::click(layer,locate(QPointF(70,50)));
        check(fixture.view->blue->clicks==blue+1 && fixture.view->red->clicks==red+1,
              "independently sized header buttons use the same inverse map as their rendered positions");
        bar->setValue(bar->maximum());TimelineViewportFixture::settle();
        check(viewport->headerOffset()>0. && locate(QPointF(30,50)).x()<0.,"blue/red buttons scroll out with the complete header at every tested body scale");
        check(layer->rect().contains(bar->geometry()) && bar->width()>=70 && bar->maximum()>0,
              "compact header retains a usable scrollbar rather than clipping controls without recovery");
        check(fixture.view->tracksScene==trackScene && fixture.view->transform()==transform,"independent header size never changes native track model or world transform");
    }
    scale.setValue(3.);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();bar->setValue(0);TimelineViewportFixture::settle();
    auto headerImage=[&]{const auto image=layer->grab().toImage();return image.copy(QRect(0,0,qRound(viewport->headerWidth()*3*image.devicePixelRatioF()),qRound(40*3*image.devicePixelRatioF())));};
    const int compactClock=paintedInkHeight(headerImage(),QColor(255,180,0));
    button->menu()->popup(button->mapToGlobal(QPoint()));TimelineViewportFixture::settle();
    auto typed=button->menu()->findChild<QSpinBox *>("openGatePanelScalePercent");
    check(typed && typed->maximum()==300,"header scale menu reuses the typed smooth scale control with a row-fit limit");
    if(typed)typed->setValue(147);button->menu()->hide();TimelineViewportFixture::settle();
    check(qAbs(scale.snapshot()["timeline_header_scale"].toDouble()-1.47)<.001 && viewport->headerScale()==1.47,
          "header scale menu saves an independent intermediate percentage through existing persistence");
    check(paintedInkHeight(headerImage(),QColor(255,180,0))>compactClock,"timecode glyphs grow with the independent header content size");
    auto saved=scale.snapshot();check(scale.suspend(),"header-scaled native return releases every presentation lease");
    Unbound::PanelContentScale restored;restored.restoreState(saved);check(restored.apply(&fixture.timeline),"fresh owner restores header scaling, width, scroll and pin state");
    TimelineViewportFixture::settle();check(restored.snapshot()==saved,"independent header state survives an unsaved workspace round trip exactly");
    auto next=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    auto tracks=qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget());
    const auto pinnedWidth=next->headerWidth();tracks->horizontalScrollBar()->setValue(100);TimelineViewportFixture::settle();
    check(qAbs(next->headerWidth()-pinnedWidth)<.01,"whole-header pin retains its width during native time panning");
    next->presentation()->findChild<QToolButton *>("openGateTimelinePinTrackControls")->click();TimelineViewportFixture::settle();
    check(next->headerWidth()<pinnedWidth,"unpin lets the whole left area scroll with native time, without selectively freezing buttons");
    tracks->horizontalScrollBar()->setValue(500);TimelineViewportFixture::settle();
    check(next->headerWidth()==0. && next->presentation()->findChild<QToolButton *>("openGateTimelinePinTrackControls")->isVisible(),
          "fully scrolled-away header retains its pin/size controls so it can always be recovered");
    next->presentation()->findChild<QToolButton *>("openGateTimelinePinTrackControls")->click();TimelineViewportFixture::settle();
    check(next->headerWidth()>0.,"pin recalls the complete track-control area without resetting its size");
    restored.suspend();
}
}
