// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QMimeData>
#include <QGraphicsSceneDragDropEvent>
#include <cstdio>
#include "../../../src/core/panelcontentscale.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include "../../core/workspace-callers/timelinesourceeditchecks.h"
using namespace OpenGateTest;
class NativeDropPolicy : public QObject {
protected:
    bool eventFilter(QObject *,QEvent *e) override {
        if(e->type()!=QEvent::GraphicsSceneDragEnter && e->type()!=QEvent::GraphicsSceneDragMove && e->type()!=QEvent::GraphicsSceneDrop)return false;
        auto drag=static_cast<QGraphicsSceneDragDropEvent *>(e);
        if(drag->mimeData()->hasFormat("application/x-opengate-test-clip"))return false;
        drag->ignore();return true;
    }
};
class NativeDropZone : public QGraphicsRectItem {
public:
    int enters=0,moves=0,drops=0,leaves=0;QPointF point;QPoint cursor;const QMimeData *payload=nullptr;
    NativeDropZone():QGraphicsRectItem(0,0,300,90){setZValue(100);setBrush(Qt::darkGray);setPen(Qt::NoPen);setAcceptDrops(true);}
protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *e) override {
        ++enters;if(!e->mimeData()->hasFormat("application/x-opengate-test-clip")){e->ignore();return;}
        e->setDropAction(Qt::CopyAction);e->accept();setBrush(Qt::yellow);
    }
    void dragMoveEvent(QGraphicsSceneDragDropEvent *e) override {
        ++moves;point=e->scenePos();payload=e->mimeData();
        if(e->widget())cursor=e->widget()->mapFromGlobal(e->screenPos());
        e->setDropAction(Qt::CopyAction);e->accept();setBrush(Qt::yellow);
    }
    void dropEvent(QGraphicsSceneDragDropEvent *e) override {
        ++drops;point=e->scenePos();payload=e->mimeData();e->setDropAction(Qt::CopyAction);e->accept();
        auto inserted=scene()->addRect(0,0,140,70,QPen(Qt::NoPen),QBrush(Qt::green));inserted->setPos(100,0);inserted->setZValue(200);
        setBrush(Qt::darkGray);
    }
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *e) override {++leaves;setBrush(Qt::darkGray);e->accept();}
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir files;qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;auto check=[&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        TimelineViewportFixture fixture;NativeDropPolicy policy;fixture.view->sequence->installEventFilter(&policy);
        auto zone=new NativeDropZone;fixture.view->tracksScene->addItem(zone);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});
        scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto layer=content->presentation();const QPoint at(qRound((content->headerWidth()+40)*factor),qRound(65*factor));
        QMimeData mime;mime.setData("application/x-opengate-test-clip","source-clip-42");
        auto pixel=[&](int x){const auto image=layer->grab().toImage();return image.pixelColor((QPointF((content->headerWidth()+x)*factor,65*factor)*image.devicePixelRatioF()).toPoint());};
        pixel(40);QDragEnterEvent enter(at,Qt::CopyAction|Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(layer,&enter);check(enter.isAccepted(),"native Timeline accepts the original drag entry");
        QDragMoveEvent move(at,Qt::CopyAction|Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(layer,&move);TimelineViewportFixture::settle();
        check(move.isAccepted() && zone->moves==1 && zone->payload==&mime,"native drag move reaches the real scene once with original MIME data");
        check(pixel(40)==QColor(Qt::yellow),"drag preview appears before drop without resize, maximize or extra click");
        if(QLineF(zone->point,QPointF(40,25)).length()>=3 || (zone->cursor-QPoint(40,25)).manhattanLength()>=3)
            std::fprintf(stderr,"drag coordinate scale %.2f event=%.1f,%.1f mapped-screen=%d,%d\n",factor,zone->point.x(),zone->point.y(),zone->cursor.x(),zone->cursor.y());
        check(QLineF(zone->point,QPointF(40,25)).length()<3 && (zone->cursor-QPoint(40,25)).manhattanLength()<3,
              "native scene and global event coordinates agree at every scale");
        QDropEvent drop(at,Qt::CopyAction|Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(layer,&drop);TimelineViewportFixture::settle();
        if(zone->drops!=1)std::fprintf(stderr,"drop scale %.2f enter=%d move=%d drop=%d accepted=%d\n",factor,zone->enters,zone->moves,zone->drops,drop.isAccepted());
        check(drop.isAccepted() && drop.dropAction()==Qt::CopyAction && zone->drops==1,"native drop inserts once and returns its accepted action");
        check(pixel(130)==QColor(Qt::green),"inserted native clip paints immediately without another command");
        QDragEnterEvent again(at,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&again);
        QDragMoveEvent preview(at,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&preview);
        TimelineViewportFixture::settle();check(pixel(40)==QColor(Qt::yellow),"a subsequent drag gets a fresh native preview");
        QDragLeaveEvent leave;QApplication::sendEvent(layer,&leave);TimelineViewportFixture::settle();
        check(zone->leaves==1 && pixel(40)==QColor(Qt::darkGray),"canceling a drag removes the native preview immediately");
        QMimeData rejected;rejected.setData("application/x-unsupported-test","ignored");
        QDragEnterEvent badEnter(at,Qt::CopyAction,&rejected,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&badEnter);
        QDragMoveEvent badMove(at,Qt::CopyAction,&rejected,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&badMove);
        QDropEvent badDrop(at,Qt::CopyAction,&rejected,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&badDrop);
        check(!badMove.isAccepted() && !badDrop.isAccepted() && zone->drops==1,"native rejection is preserved and cannot insert another clip");
        const auto enters=zone->enters;fixture.view->viewport()->setEnabled(false);
        QDragEnterEvent disabled(at,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&disabled);
        check(!disabled.isAccepted() && zone->enters==enters,"a disabled native viewport cannot accept a mapped drag");
    }
    QFile evidence(files.path()+"/events.jsonl");evidence.open(QIODevice::ReadOnly);int dropFrames=0;
    for(const auto &line:evidence.readAll().split('\n')) {
        const auto record=QJsonDocument::fromJson(line).object();
        if(record["event"]=="timeline.command.presented" && record["sources"].toArray().contains("native-timeline-drop"))
            ++dropFrames;
    }
    check(dropFrames==5,"each completed native drop records its frame without per-move frame diagnostics");
    timelineSourceEditChecks(check);
    std::printf("timeline drop failures=%d\n",failures);return failures?1:0;
}
