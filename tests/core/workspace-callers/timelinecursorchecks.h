// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsSceneHoverEvent>
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinecontentviewport.h"

namespace OpenGateTest {
class EditHoverItem : public QGraphicsRectItem {
public:
    EditHoverItem() : QGraphicsRectItem(0,0,190,100) {setAcceptHoverEvents(true);setZValue(50);}
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override {hoverMoveEvent(event);}
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        setCursor(event->pos().x()<50?QCursor(Qt::SplitHCursor):QCursor(Qt::SizeHorCursor));
    }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override {unsetCursor();}
};
template<class Check> void timelineCursorChecks(Unbound::TimelineContentViewport *viewport,TimelineSourceView *source,Check check) {
    auto layer=viewport->presentation();
    auto tracks=qobject_cast<QGraphicsView *>(source->tracksProxy->widget());
    auto item=new EditHoverItem;source->tracksScene->addItem(item);
    const auto width=viewport->retainedHeaderWidth(),offset=viewport->headerOffset();const bool pinned=viewport->headerPinned();
    auto hover=[&](QPointF point) {
        QMouseEvent event(QEvent::MouseMove,point,layer->mapToGlobal(point.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(layer,&event);
    };
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        viewport->configure(factor,width,offset,pinned);TimelineViewportFixture::settle();
        const QPointF edge((viewport->headerWidth()+30)*factor,65*factor);
        hover(edge);
        check(layer->cursor().shape()==Qt::SplitHCursor,"native edit-hover handler cursor reaches the scaled presentation");
        hover(QPointF((viewport->headerWidth()+90)*factor,65*factor));
        check(layer->cursor().shape()==Qt::SizeHorCursor,"moving from trim to roll hit area updates the native cursor");
        QPixmap image(18,18);image.fill(Qt::transparent);{QPainter p(&image);p.fillRect(3,2,8,12,Qt::magenta);}
        const QCursor custom(image,4,6);
        tracks->viewport()->setCursor(custom);TimelineViewportFixture::settle();
        check(layer->cursor().shape()==Qt::BitmapCursor && layer->cursor().hotSpot()==custom.hotSpot() &&
              layer->cursor().pixmap().toImage()==custom.pixmap().toImage(),
              "native cursor changes without a mouse move preserve custom trim artwork and hotspot");
        tracks->viewport()->unsetCursor();
        hover(QPointF((viewport->headerWidth()+220)*factor,65*factor));TimelineViewportFixture::settle();
        check(layer->cursor().shape()==Qt::ArrowCursor,"leaving an edit point clears the trim cursor");
        QEvent leave(QEvent::Leave);QApplication::sendEvent(layer,&leave);
        tracks->viewport()->setCursor(Qt::CrossCursor);TimelineViewportFixture::settle();
        check(!layer->testAttribute(Qt::WA_SetCursor),"native cursor changes after presentation leave cannot revive a stale cursor");
        tracks->viewport()->unsetCursor();
    }
    delete item;
    viewport->configure(2.,width,offset,pinned);TimelineViewportFixture::settle();
}
}
