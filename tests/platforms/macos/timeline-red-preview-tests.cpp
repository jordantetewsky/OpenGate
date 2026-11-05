// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QGraphicsColorizeEffect>
#include <QUndoStack>
#include <QUndoCommand>
#include <QAction>
#include <QShortcut>
#include <QShowEvent>
#include <cstdio>
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/timelineclipcolorpreview.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include "../../core/workspace-callers/clipbatchedbackgroundchecks.h"
#include "../../core/workspace-callers/clipskinpaintchecks.h"
#include "../../core/workspace-callers/clipstylepaintchecks.h"

using namespace OpenGateTest;
class MoveClip : public QUndoCommand {
public:
    explicit MoveClip(QGraphicsItem *clip):clip(clip),before(clip->pos()){}
    void redo() override {clip->setPos(before+QPointF(8,0));}
    void undo() override {clip->setPos(before);}
private:QGraphicsItem *clip;QPointF before;
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir tmp;qputenv("UNBOUND_DIAGNOSTICS_DIR",tmp.path().toUtf8());
    qputenv("OPENGATE_COLOR_DISPLAY_FILE",(tmp.path()+"/ColorDisplay.json").toUtf8());
    int failures=0;auto check=[&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    for(bool rounded:{false,true})for(double factor:{.7,1.,1.47,2.13,3.}) {
        if(qEnvironmentVariableIsSet("OPENGATE_TEST_ONE_SCALE") && factor!=1.)continue;
        TimelineViewportFixture fixture;fixture.view->clip->hide();
        auto clip=new UiTiClipContainer;clip->rounded=rounded;clip->setPos(0,2);fixture.view->tracksScene->addItem(clip);clip->setSelected(true);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});
        scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto layer=content->presentation();auto tracks=qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget());
        auto at=[&](QPointF local){return QPoint(qRound((content->headerWidth()+local.x())*factor),qRound((40+local.y())*factor));};
        auto frame=[&]{TimelineViewportFixture::settle();return layer->grab().toImage();};
        const auto before=frame();const auto root=clip->parentItem();const auto scene=clip->scene();const auto transform=clip->transform();
        qputenv("OPENGATE_EXPERIMENTAL_RED_PREVIEW","1");
        Unbound::TimelineClipColorPreview preview(fixture.view,content);
        qunsetenv("OPENGATE_EXPERIMENTAL_RED_PREVIEW");
        check(preview.applySelected(QColor("#ff0000"))==1,"selected native clip accepts a temporary preview");
        const auto red=frame();
        if(factor==1.){before.save("build/red-preview-before.png");red.save("build/red-preview-after.png");}
        auto pixel=[&](const QImage &im,QPointF point){return im.pixelColor((QPointF(at(point))*im.devicePixelRatioF()).toPoint());};
        if(pixel(red,{200,20})!=QColor("#ff0000"))std::fprintf(stderr,"scale %.2f bodycolor=%s expected red\n",factor,qPrintable(pixel(red,{200,20}).name()));
        check(pixel(red,{200,20})==QColor("#ff0000"),"native solid clip fill renders red at every content scale");
        check(pixel(red,{20,15})==pixel(before,{20,15}) && pixel(red,{20,15})==clip->fill,"same-colored thumbnail pixels are unchanged");
        const QRect preserved((QPointF(at({8,40}))*red.devicePixelRatioF()).toPoint(),(QPointF(at({168,85}))*red.devicePixelRatioF()).toPoint());
        check(red.copy(preserved)==before.copy(preserved),"waveform paths and native text pixels remain unchanged");
        check(clip->scene()==scene && clip->parentItem()==root && clip->transform()==transform && clip->fill==QColor(30,130,180),
              "preview does not mutate native identity, scene, transform or stored color");
        check(preview.applySelected(QColor("#e15da8"))==1 && pixel(frame(),{200,20})==QColor("#e15da8"),
              "the fill path accepts arbitrary RGB colors, not a hard-coded Red alias");
        preview.applySelected(QColor("#ff0000"));frame();
        TimelineViewportFixture::click(layer,at({195,20}));check(clip->presses==1,"native clip selection receives the same displayed click");
        QUndoStack history;QAction edit("Native move",tracks),undo("Native Undo",tracks),redo("Native Redo",tracks);
        QObject::connect(&edit,&QAction::triggered,tracks,[&]{history.push(new MoveClip(clip));});
        QObject::connect(&undo,&QAction::triggered,&history,&QUndoStack::undo);
        QObject::connect(&redo,&QAction::triggered,&history,&QUndoStack::redo);
        // Discovery intentionally uses the shared activity path, with queued native handlers.
        tracks->addAction(&edit);tracks->addAction(&undo);tracks->addAction(&redo);TimelineViewportFixture::settle();
        const auto stable=frame();edit.trigger();const auto moved=frame();
        check(clip->x()==8 && moved!=stable,"native command changes actual position and displayed preview");
        undo.trigger();check(clip->x()==0 && frame()==stable,"native Undo restores model and visible pixels with preview attached");
        redo.trigger();check(clip->x()==8 && frame()==moved,"native Redo restores model and visible pixels with preview attached");
        QShortcut shortcut(QKeySequence(Qt::Key_F8),tracks);int calls=0;
        QObject::connect(&shortcut,&QShortcut::activated,tracks,[&]{++calls;clip->setWidth(210);});
        QShortcutEvent event(shortcut.key(),shortcut.id(),false);QApplication::sendEvent(&shortcut,&event);
        const auto trimmed=frame();check(calls==1 && clip->bounds.width()==210 && trimmed!=moved,"native shortcut and geometry edits update once while previewed");
        preview.clear();const auto cleared=frame();
        check(!clip->graphicsEffect() && pixel(cleared,{200,20})==clip->fill,"clear restores native painting without altering the native clip");
        clip->setSelected(true);auto nativeEffect=new QGraphicsColorizeEffect;clip->setGraphicsEffect(nativeEffect);
        check(preview.applySelected(Qt::red)==0 && clip->graphicsEffect()==nativeEffect,"existing native effects are never replaced");
        clip->setGraphicsEffect(nullptr);preview.applySelected(Qt::red);
        auto replacement=new QGraphicsColorizeEffect;clip->setGraphicsEffect(replacement);preview.clear();
        check(clip->graphicsEffect()==replacement,"clearing an old lease preserves a subsequently installed native effect");
        clip->setGraphicsEffect(nullptr);preview.applySelected(Qt::red);delete clip;preview.clear();
    }
    clipBatchedBackgroundChecks(check);
    clipSkinPaintChecks(check);
    clipStylePaintChecks(check);
    {
        TimelineViewportFixture fixture;fixture.view->clip->hide();auto clip=new UiTiClipContainer;
        clip->rasterOnly=true;clip->setCacheMode(QGraphicsItem::DeviceCoordinateCache);fixture.view->tracksScene->addItem(clip);clip->setSelected(true);
        Unbound::PanelContentScale scale;scale.restoreState({{"timeline_header_width",160.}});scale.apply(&fixture.timeline);
        TimelineViewportFixture::settle();auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        Unbound::TimelineClipColorPreview preview(fixture.view,content);preview.applySelected(Qt::red);
        content->presentation()->grab();TimelineViewportFixture::settle();
        const auto image=content->presentation()->grab().toImage();
        check(!clip->graphicsEffect() && clip->cacheMode()==QGraphicsItem::DeviceCoordinateCache && clip->fill==QColor(30,130,180) &&
            image.pixelColor((QPointF(content->headerWidth()+200,60)*image.devicePixelRatioF()).toPoint())==clip->fill,
            "unsupported raster-only source restores native painting and never tints thumbnail/image pixels or changes native color");
    }
    if(failures){QFile::remove("build/view-fill-fail-events.jsonl");QFile::copy(tmp.path()+"/events.jsonl","build/view-fill-fail-events.jsonl");}
    if (const auto receipt=qEnvironmentVariable("OPENGATE_COLOR_PAINT_TRACE_RECEIPT");!receipt.isEmpty()) {
        QFile::remove(receipt);
        check(QFile::copy(tmp.path()+"/events.jsonl",receipt),"bounded paint evidence receipt is retained");
    }
    std::printf("timeline red preview failures=%d\n",failures);return failures?1:0;
}
