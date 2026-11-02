// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsSceneHoverEvent>
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"

namespace OpenGateTest {
class NativeTrimItem : public QGraphicsRectItem {
public:
    NativeTrimItem(QColor color):QGraphicsRectItem(0,0,100,90) {setBrush(color);setPen(Qt::NoPen);setZValue(80);setAcceptHoverEvents(true);}
    QGraphicsRectItem *neighbor=nullptr;
    QPointF cursorPoint;
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override {hoverMoveEvent(event);}
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        setCursor(event->modifiers().testFlag(Qt::AltModifier)?Qt::SplitHCursor:Qt::SizeHorCursor);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        original=rect();following=neighbor?neighbor->rect():QRectF();start=event->scenePos();
        left=event->pos().x()<rect().left()+4;roll=!left && event->modifiers().testFlag(Qt::AltModifier);event->accept();
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        auto next=original;const double delta=event->scenePos().x()-start.x();
        if(left)next.setLeft(original.left()+delta);else next.setRight(original.right()+delta);
        setRect(next);
        if(roll && neighbor){auto other=following;other.setLeft(next.right());neighbor->setRect(other);}
        if(auto widget=event->widget())if(auto view=qobject_cast<QGraphicsView *>(widget->parentWidget()))
            cursorPoint=view->mapToScene(widget->mapFromGlobal(QCursor::pos()));
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {event->accept();}
private:
    QRectF original,following;QPointF start;bool left=false,roll=false;
};
template<class Check> void timelineEditPointChecks(Check check) {
    TimelineViewportFixture fixture;Unbound::PanelContentScale scale;
    scale.restoreState({{"interface_scale",1.},{"timeline_header_width",160.},{"timeline_header_scale",.7}});
    scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
    auto viewport=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    if(!viewport)return;auto layer=viewport->presentation();
    auto clip=new NativeTrimItem(Qt::yellow);auto neighbor=new QGraphicsRectItem(100,0,100,90);
    neighbor->setBrush(Qt::blue);neighbor->setPen(Qt::NoPen);neighbor->setZValue(70);clip->neighbor=neighbor;
    fixture.view->tracksScene->addItem(clip);fixture.view->tracksScene->addItem(neighbor);
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        scale.setValue(factor);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto at=[&](double x){return QPointF((viewport->headerWidth()+x)*factor,100*factor);};
        auto send=[&](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers modifiers) {
            const auto physical=QCursor::pos();fixture.timeline.move(fixture.timeline.pos()+physical-layer->mapToGlobal(point.toPoint()));
            TimelineViewportFixture::settle();QMouseEvent event(type,point,physical,button,buttons,modifiers);QApplication::sendEvent(layer,&event);
            TimelineViewportFixture::settle();
        };
        auto color=[&](double x){const auto image=layer->grab().toImage();return image.pixelColor((at(x)*image.devicePixelRatioF()).toPoint());};
        for(int mode=0;mode<3;++mode) {
            clip->setRect(0,0,100,90);neighbor->setRect(100,0,100,90);TimelineViewportFixture::settle();
            const double start=mode==0?1.:99.,finish=mode==0?21.:mode==1?79.:119.;
            const auto modifier=mode==2?Qt::AltModifier:Qt::NoModifier;
            send(QEvent::MouseMove,at(start),Qt::NoButton,Qt::NoButton,modifier);
            check(layer->cursor().shape()==(mode==2?Qt::SplitHCursor:Qt::SizeHorCursor),"displayed edit-point hover uses the native trim/roll cursor before clicking");
            send(QEvent::MouseButtonPress,at(start),Qt::LeftButton,Qt::LeftButton,modifier);
            send(QEvent::MouseMove,at(finish),Qt::NoButton,Qt::LeftButton,modifier);
            check(QLineF(clip->cursorPoint,QPointF(finish,60)).length()<3.,"native edit-point handler reads the same live cursor location as the mapped mouse event");
            check(qAbs((mode==0?clip->rect().left():clip->rect().right())-(mode==0?20.:mode==1?80.:120.))<1.,
                  "trim-in, trim-out and roll grabs change the actual native item boundary at the displayed edit point");
            if(mode==2)check(qAbs(neighbor->rect().left()-120.)<1. && color(110)==QColor(Qt::yellow) && color(130)==QColor(Qt::blue),
                            "native roll updates both adjacent clip boundaries and visible colors during the grab");
            else check(color(mode==0?5.:90.)!=QColor(Qt::yellow) && color(mode==0?30.:70.)==QColor(Qt::yellow),
                       "native trim paints the vacated and remaining clip area before release, without maximize or resize");
            send(QEvent::MouseButtonRelease,at(finish),Qt::LeftButton,Qt::NoButton,modifier);
        }
    }
    scale.suspend();
}
}
