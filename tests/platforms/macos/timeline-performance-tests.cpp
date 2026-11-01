// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>
#include <cstdio>
#include "../../../src/core/timelinecontentviewport.h"
#include "../../../src/core/latencystats.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"

class DenseClip : public QGraphicsItem {
public:
    static quint64 paints;
    QRectF boundingRect() const override {return {0,0,28,36};}
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {
        ++paints;p->fillRect(boundingRect(),QColor(35,90,115));
        p->setPen(QColor(90,150,170));
        for(int i=0;i<14;++i)p->drawLine(QPointF(i*2,7),QPointF(i*2,12+(i%7)*2));
        p->setPen(Qt::white);p->drawText(QPointF(2,33),"clip");
    }
};
quint64 DenseClip::paints=0;
class PaintCounter : public QObject {
public:
    int count=0;
    bool eventFilter(QObject *,QEvent *event) override {if(event->type()==QEvent::Paint)++count;return false;}
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir temporary;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",temporary.path().toUtf8());
    QJsonObject results{{"kind","synthetic Qt Timeline UI workload; no media decoding or native GPU playback"}};
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        OpenGateTest::TimelineViewportFixture fixture;fixture.timeline.resize(1440,900);
        for(int row=0;row<24;++row)for(int clip=0;clip<120;++clip) {
            auto item=new DenseClip;item->setPos(clip*32,row*40);fixture.view->tracksScene->addItem(item);
        }
        Unbound::TimelineContentViewport viewport(fixture.view,&fixture.timeline);
        viewport.setGeometry(0,31,1440,850);viewport.show();viewport.configure(factor,160,0,true);
        auto layer=viewport.presentation();
        auto playhead=new QGraphicsRectItem(0,0,2,fixture.view->viewport()->height());
        playhead->setBrush(Qt::red);playhead->setPen(Qt::NoPen);playhead->setZValue(1000);
        fixture.view->sequence->addItem(playhead);playhead->setX(300);
        OpenGateTest::TimelineViewportFixture::settle();
        QImage canvas(layer->size()*layer->devicePixelRatioF(),QImage::Format_ARGB32_Premultiplied);
        canvas.setDevicePixelRatio(layer->devicePixelRatioF());canvas.fill(Qt::black);
        auto paint=[&](QRegion region) {QPainter p(&canvas);layer->render(&p,QPoint(),region);};
        for(int i=0;i<3;++i)paint(layer->rect());
        Unbound::LatencyStats exposes,moves,inputs,loops;quint64 exposePaints=0,movePaints=0;
        const auto exposedBefore=DenseClip::paints;
        for(int i=0;i<20;++i) {QElapsedTimer timer;timer.start();paint(layer->rect());exposes.add(timer.nsecsElapsed());}
        exposePaints=DenseClip::paints-exposedBefore;
        const auto movingBefore=DenseClip::paints;
        for(int i=0;i<40;++i) {
            QElapsedTimer whole;whole.start();
            const double previous=playhead->x();playhead->setX(300+i*2);
            for(int event=0;event<3;++event)QApplication::processEvents();
            const int left=qFloor((qMin(previous,playhead->x())-260+viewport.headerWidth()-4)*factor);
            const int right=qCeil((qMax(previous,playhead->x())-260+viewport.headerWidth()+7)*factor);
            QElapsedTimer timer;timer.start();paint(QRect(left,0,right-left,layer->height()));moves.add(timer.nsecsElapsed());loops.add(whole.nsecsElapsed());
        }
        movePaints=DenseClip::paints-movingBefore;
        const QPointF point((viewport.headerWidth()+40)*factor,65*factor);
        for(int i=0;i<300;++i) {
            QMouseEvent event(QEvent::MouseMove,point,layer->mapToGlobal(point.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
            QElapsedTimer timer;timer.start();QApplication::sendEvent(layer,&event);inputs.add(timer.nsecsElapsed());
        }
        Unbound::LatencyStats panning;
        PaintCounter layerPaints;layer->installEventFilter(&layerPaints);
        const auto panClipBefore=DenseClip::paints;
        auto tracks=qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget());
        for(int i=0;i<20;++i) {
            QElapsedTimer timer;timer.start();tracks->horizontalScrollBar()->setValue(i*8);
            for(int event=0;event<3;++event)QApplication::processEvents();
            paint(layer->rect());panning.add(timer.nsecsElapsed());
        }
        QJsonArray budgets;
        for(double fps:{23.976,24.,25.,29.97,30.,50.,59.94,60.,100.,119.88,120.,144.,240.})
            budgets.append(QJsonObject{{"fps",fps},{"budget_ms",1000./fps},{"measured_ui_p95_budget_share",loops.summary()["p95_ms"].toDouble()*fps/1000.}});
        QJsonObject row{{"scale",factor},{"display",QJsonArray{layer->width(),layer->height()}},{"dpr",layer->devicePixelRatioF()},
            {"dense_clips",2880},{"expose",exposes.summary()},{"playhead",moves.summary()},{"event_loop_and_paint",loops.summary()},{"input",inputs.summary()},{"panning",panning.summary()},{"pan_layer_paints",layerPaints.count},{"pan_clip_paints",double(DenseClip::paints-panClipBefore)},
            {"expose_clip_paints",double(exposePaints)},{"playhead_clip_paints",double(movePaints)},{"frame_budgets",budgets}};
        results[QString::number(factor)]=row;
        std::printf("scale %.2f expose p95 %.3f ms moving p95 %.3f ms loop p95 %.3f ms input p95 %.3f ms pan p95 %.3f ms clip-paints %llu/%llu pan paints %d clips %llu\n",factor,
            exposes.summary()["p95_ms"].toDouble(),moves.summary()["p95_ms"].toDouble(),loops.summary()["p95_ms"].toDouble(),inputs.summary()["p95_ms"].toDouble(),panning.summary()["p95_ms"].toDouble(),
            static_cast<unsigned long long>(exposePaints),static_cast<unsigned long long>(movePaints),layerPaints.count,static_cast<unsigned long long>(DenseClip::paints-panClipBefore));
    }
    const auto output=qEnvironmentVariable("OPENGATE_PERF_OUTPUT");
    if(!output.isEmpty()) {QSaveFile file(output);if(!file.open(QIODevice::WriteOnly))return 2;file.write(QJsonDocument(results).toJson());if(!file.commit())return 2;}
    return 0;
}
