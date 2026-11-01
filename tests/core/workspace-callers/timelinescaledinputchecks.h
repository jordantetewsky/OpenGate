// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinecontentviewport.h"
namespace OpenGateTest {
template<class Check> void timelineScaledInputChecks(Unbound::TimelineContentViewport *viewport,TimelineSourceView *source,Check check) {
    auto layer=viewport->presentation();
    const auto width=viewport->retainedHeaderWidth(),offset=viewport->headerOffset();const bool pinned=viewport->headerPinned();
    for(double factor:{.7,.73,.85,1.,1.25,1.47,2.,2.13,2.63,3.}) {
    viewport->configure(factor,width,offset,pinned);TimelineViewportFixture::settle();
    const QPointF start((viewport->headerWidth()+40)*factor,65.*factor);
    const QPointF end(20.,layer->height()-5.);
    const auto before=source->clip->moves;
    QMouseEvent press(QEvent::MouseButtonPress,start,layer->mapToGlobal(start.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(layer,&press);
    QMouseEvent move(QEvent::MouseMove,end,layer->mapToGlobal(end.toPoint()),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(layer,&move);
    QMouseEvent release(QEvent::MouseButtonRelease,end,layer->mapToGlobal(end.toPoint()),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(layer,&release);
    const QPointF expected(40.+(end.x()-start.x())/factor,25.+(end.y()-start.y())/factor);
    check(source->clip->moves==before+1 && QLineF(source->clip->lastDrag,expected).length()<3.,
          "scaled native clip drag stays continuous across the pinned header and independent footer bands");
    source->clip->setCursor(Qt::SizeHorCursor);source->clip->setAcceptHoverEvents(true);
    QMouseEvent hover(QEvent::MouseMove,start,layer->mapToGlobal(start.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(layer,&hover);
    check(layer->cursor().shape()==Qt::SizeHorCursor,"scaled pointer retains the native clip cursor after header narrowing");
    source->clip->unsetCursor();
    }
    viewport->configure(2.,width,offset,pinned);TimelineViewportFixture::settle();
}
}
