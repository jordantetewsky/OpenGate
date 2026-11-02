// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinecontentviewport.h"

namespace OpenGateTest {
class UpdatingNativePaint : public QGraphicsRectItem {
public:
    UpdatingNativePaint():QGraphicsRectItem(0,0,180,90){setZValue(100);}
    bool change=false;QColor color=Qt::yellow;
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {
        p->fillRect(rect(),color);
        if(!change)return;change=false;color=Qt::cyan;update();
        // Native rendering can drain pending model messages while awaiting media.
        // Use public event delivery to emit the actual queued scene notification.
        for(int i=0;i<4;++i)QCoreApplication::sendPostedEvents(scene(),QEvent::MetaCall);
    }
};
template<class Check> void timelineCaptureDamageChecks(Check check) {
    TimelineViewportFixture fixture;
    fixture.view->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget())->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    auto item=new UpdatingNativePaint;fixture.view->tracksScene->addItem(item);
    Unbound::TimelineContentViewport viewport(fixture.view,&fixture.timeline);viewport.setGeometry(0,31,900,500);viewport.show();
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        viewport.configure(factor,160,0,true);TimelineViewportFixture::settle();
        item->color=Qt::yellow;item->change=true;item->update();TimelineViewportFixture::settle();
        auto layer=viewport.presentation();layer->grab();TimelineViewportFixture::settle();
        const auto image=layer->grab().toImage();const auto at=QPointF((viewport.headerWidth()+40)*factor,65*factor)*image.devicePixelRatioF();
        check(item->color==Qt::cyan && image.pixelColor(at.toPoint())==QColor(Qt::cyan),
              "native model damage emitted during raster capture is retained and presented on the next frame without resize or maximize");
    }
}
}
