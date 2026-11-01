// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinecontentviewport.h"

namespace OpenGateTest {
// Model a native paint path that drops its painter clip at high backing DPI.
class OverflowAudioItem : public QGraphicsRectItem {
public:
    OverflowAudioItem():QGraphicsRectItem(0,0,700,100) { setPos(0,170);setZValue(100);setPen(Qt::NoPen); }
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {
        p->save();p->setClipping(false);
        p->fillRect(QRectF(0,p->device()->devicePixelRatioF()>1.?-110.:0.,700,100),Qt::magenta);
        p->restore();
    }
};
template<class Check> void timelineSectionPaintChecks(Check check) {
    TimelineViewportFixture fixture;
    auto audio=new QGraphicsView(new QGraphicsScene(&fixture.timeline));
    audio->setObjectName("AudioTracksView");TimelineSourceView::setup(audio);
    auto proxy=fixture.view->sequence->addWidget(audio);proxy->setGeometry(QRectF(260,170,500,100));
    fixture.view->sequence->addItem(new OverflowAudioItem);
    Unbound::TimelineContentViewport viewport(fixture.view,&fixture.timeline);
    viewport.setGeometry(0,31,900,500);viewport.show();
    for(double factor:{.7,.73,.85,1.,1.25,1.47,2.,2.13,2.63,3.}) {
        viewport.configure(factor,160.,0.,true);TimelineViewportFixture::settle();
        const auto image=viewport.presentation()->grab().toImage();
        const auto point=QPointF(200*factor,100*factor)*image.devicePixelRatioF();
        check(image.pixelColor(point.toPoint())!=QColor(Qt::magenta),
              "audio paint cannot escape its section into video at increased backing resolution");
    }
}
template<class Check> void timelineOverflowChecks(Check check) {
    QWidget host;host.setAttribute(Qt::WA_DontShowOnScreen);host.resize(900,350);
    auto source=new TimelineSourceView(&host);
    source->setMinimumHeight(480);
    Unbound::TimelineContentViewport viewport(source,&host);
    viewport.setGeometry(host.rect());host.show();viewport.show();
    for(double factor:{.7,.73,.85,1.,1.25,1.47,2.,2.13,2.63,3.}) {
        viewport.configure(factor,140.,0.,true);TimelineViewportFixture::settle();
        auto layer=viewport.presentation();
        auto bar=layer->findChild<QScrollBar *>("openGateTimelineHeaderScroll");
        check(bar && bar->isVisible() && bar->height()>=16 && layer->rect().contains(bar->geometry()),
              "header scrollbar reserves a usable visible row despite native height constraints at every scale");
        check(bar && bar->maximum()>0,"narrowed header has reachable overflow at every scale");
        if(bar) {
            const auto image=bar->grab().toImage();
            bool painted=false;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
                const auto color=image.pixelColor(x,y);
                painted|=qAbs(color.red()-96)<3 && qAbs(color.green()-102)<3 && qAbs(color.blue()-110)<3;
            }
            check(painted,"header scrollbar thumb is actually painted without waiting for macOS transient scrolling");
        }
    }
    timelineSectionPaintChecks(check);
}
}
