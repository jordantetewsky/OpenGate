// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>
#include <QUndoStack>
#include <QUndoCommand>
#include <QTemporaryDir>
#include <QSaveFile>
#include "timelineviewportfixtures.h"
#include "nativepresetfixture.h"
#include "../../../src/core/panelcommandcatalog.h"
#include "../../../src/core/workflowcommandresults.h"

namespace OpenGateTest {
class NativeWidgetShortcut : public QWidget {
public:
    using QWidget::QWidget;
    std::function<void()> execute;
protected:
    bool event(QEvent *event) override {
        if(event->type()!=QEvent::Shortcut)return QWidget::event(event);
        QTimer::singleShot(0,this,[this]{if(execute)execute();});return true;
    }
};
class NativeCommandPaint : public QGraphicsRectItem {
public:
    QColor state=Qt::yellow;
    std::function<void()> duringDrag,repaint;
    NativeCommandPaint():QGraphicsRectItem(0,0,180,90) {setZValue(100);}
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {p->fillRect(rect(),state);}
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {event->accept();}
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {state=Qt::darkGreen;if(duringDrag)duringDrag();if(repaint)repaint();else if(event->widget())event->widget()->update();event->accept();}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {event->accept();}
};
class NativePaintEdit : public QUndoCommand {
public:
    NativePaintEdit(NativeCommandPaint *item,QWidget *viewport,QColor next):item(item),viewport(viewport),before(item->state),after(next) {}
    void redo() override {apply(after);}
    void undo() override {apply(before);}
private:
    void apply(QColor color) {item->state=color;viewport->update();}
    NativeCommandPaint *item;QWidget *viewport;QColor before,after;
};
template<class Check> void timelineCommandPresentationChecks(Check check) {
    QTemporaryDir files;const auto previousPreset=qgetenv("OPENGATE_PRESET_DIRECTORY");
    QDir().mkpath(files.path()+"/preset");QDir().mkpath(files.path()+"/workflows");
    qputenv("OPENGATE_PRESET_DIRECTORY",(files.path()+"/preset").toUtf8());
    auto write=[&](const QString &name,const QByteArray &bytes){QSaveFile f(files.path()+"/"+name);f.open(QIODevice::WriteOnly);f.write(bytes);f.commit();};
    write("preset/config.user.xml","<Config><CustomKeyboardPreset>Active</CustomKeyboardPreset></Config>");
    write("preset/keyboard.preset.xml",nativePresetFixture({{"OpenGate - Refresh Timeline Test",{QKeySequence(Qt::Key_F6)}}}));
    QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1100,800);
    QWidget edit(&main);edit.setObjectName("WStackPage_Conform");edit.resize(main.size());edit.show();
    TimelineViewportFixture fixture(&edit);fixture.timeline.show();main.show();
    Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",1.},{"timeline_header_width",160.}});
    check(scale.apply(&fixture.timeline),"command/result fixture uses the production Timeline adapter");TimelineViewportFixture::settle();
    auto viewport=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    if(!viewport)return;
    auto layer=viewport->presentation();auto tracks=qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget());
    auto item=new NativeCommandPaint;fixture.view->tracksScene->addItem(item);
    auto playhead=fixture.view->sequence->addRect(0,0,2,400,QPen(Qt::NoPen),QBrush(Qt::red));playhead->setX(350);playhead->setZValue(1000);
    item->duringDrag=[&]{playhead->moveBy(1,0);};
    item->repaint=[&]{fixture.view->viewport()->update();};
    Unbound::PanelCommandCatalog commands(&main,[]{return true;});
    int ownCalls=0;
    commands.add({"fixture.timeline-visible","Refresh Timeline Test",[]{return true;},[&]{++ownCalls;item->state=Qt::magenta;tracks->viewport()->update();},"frameConformTimelines"});
    QUndoStack undo;
    QAction editAction(&main);QObject::connect(&editAction,&QAction::triggered,[&]{undo.push(new NativePaintEdit(item,tracks->viewport(),Qt::green));});
    auto undoAction=undo.createUndoAction(&main);undoAction->setObjectName("editUndo");
    auto redoAction=undo.createRedoAction(&main);redoAction->setObjectName("editRedo");
    QShortcut pointer(QKeySequence(Qt::Key_Tab),&main);pointer.setContext(Qt::ApplicationShortcut);
    NativeWidgetShortcut widgetCommand(&main);widgetCommand.setGeometry(1090,790,1,1);widgetCommand.show();
    widgetCommand.grabShortcut(QKeySequence(Qt::Key_F8),Qt::ApplicationShortcut);
    int widgetCalls=0;QPoint widgetPoint;
    widgetCommand.execute=[&]{++widgetCalls;widgetPoint=tracks->viewport()->mapFromGlobal(QCursor::pos());item->state=Qt::darkBlue;fixture.view->viewport()->update();};
    QPoint rootReceived,nestedReceived;int calls=0;
    QObject::connect(&pointer,&QShortcut::activated,&main,[&] {
        QTimer::singleShot(0,&main,[&]{++calls;rootReceived=fixture.view->viewport()->mapFromGlobal(QCursor::pos());
            nestedReceived=tracks->viewport()->mapFromGlobal(QCursor::pos());});
    });
    for(double factor:{.7,1.,1.47,2.13,3.})for(double header:{0.,.7,1.}) {
        scale.setValue(factor);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto state=scale.snapshot();state["timeline_header_scale"]=header;scale.restoreState(state);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        const QPoint target(qRound((viewport->headerWidth()+40)*factor),qRound(65*factor));
        const auto physical=QCursor::pos();fixture.timeline.move(fixture.timeline.pos()+physical-layer->mapToGlobal(target));
        TimelineViewportFixture::settle();QApplication::setActiveWindow(&main);fixture.view->setFocus();
        const auto previousCalls=calls;
        QShortcutEvent event(QKeySequence(Qt::Key_Tab),pointer.id(),false);QApplication::sendEvent(&pointer,&event);
        TimelineViewportFixture::settle();
        if((rootReceived-QPoint(300,65)).manhattanLength()>=3 || (nestedReceived-QPoint(40,25)).manhattanLength()>=3)
            std::fprintf(stderr,"queued pointer scale %.2f root %d,%d nested %d,%d\n",factor,rootReceived.x(),rootReceived.y(),nestedReceived.x(),nestedReceived.y());
        check(calls==previousCalls+1 && (rootReceived-QPoint(300,65)).manhattanLength()<3 && (nestedReceived-QPoint(40,25)).manhattanLength()<3,
              "queued native pointer command reads the displayed point through root and nested native viewports");
        check(QCursor::pos()==physical,"queued native pointer mapping never moves the OS cursor");
        auto displayed=[&] {const auto image=layer->grab().toImage();return image.pixelColor((QPointF(target)*image.devicePixelRatioF()).toPoint());};
        undo.clear();item->state=Qt::yellow;tracks->viewport()->update();TimelineViewportFixture::settle();
        // Establish a rendered image before the command changes native paint data.
        displayed();editAction.trigger();TimelineViewportFixture::settle();
        check(item->state==Qt::green && displayed()==QColor(Qt::green),"native edit updates the displayed pixels without resizing or maximizing");
        undoAction->trigger();TimelineViewportFixture::settle();
        check(item->state==Qt::yellow && displayed()==QColor(Qt::yellow),"native Undo changes model and displayed pixels without scene geometry changes");
        redoAction->trigger();TimelineViewportFixture::settle();
        check(item->state==Qt::green && displayed()==QColor(Qt::green),"native Redo changes model and displayed pixels without scene geometry changes");
        QAction queued(&main);QObject::connect(&queued,&QAction::triggered,&main,[&]{item->state=Qt::cyan;tracks->viewport()->update();},Qt::QueuedConnection);
        TimelineViewportFixture::settle();queued.trigger();TimelineViewportFixture::settle();
        check(item->state==Qt::cyan && displayed()==QColor(Qt::cyan),"dynamically created queued native actions update visible pixels once their handler runs");
        const auto ownBefore=ownCalls;
        check(commands.invoke("fixture.timeline-visible","opengate-command-menu"),"OpenGate menu command uses the actual scoped command registry");TimelineViewportFixture::settle();
        check(ownCalls==ownBefore+1 && item->state==Qt::magenta && displayed()==QColor(Qt::magenta),"OpenGate menu invocation changes native state and immediately displays its result");
        item->state=Qt::cyan;tracks->viewport()->update();TimelineViewportFixture::settle();
        QApplication::setActiveWindow(&main);fixture.view->setFocus();
        QKeyEvent key(QEvent::KeyPress,Qt::Key_F6,Qt::NoModifier);QApplication::sendEvent(fixture.view,&key);TimelineViewportFixture::settle();
        if(ownCalls!=ownBefore+2 || displayed()!=QColor(Qt::magenta))
            std::fprintf(stderr,"own command scale %.2f header %.2f calls %d expected %d pixel %s focus %s\n",factor,header,ownCalls,ownBefore+2,qPrintable(displayed().name()),
                QApplication::focusWidget()?QApplication::focusWidget()->metaObject()->className():"none");
        check(ownCalls==ownBefore+2 && displayed()==QColor(Qt::magenta),"remapped OpenGate native-preset command reaches its handler once and displays native changes");
        Unbound::WorkflowCommandResults results(&main,files.path()+"/workflows",[](const QJsonObject &result){
            if(result["success"].toBool())Unbound::NativeCommandActivity::shared()->completed(result["command"].toString());
        });
        item->state=Qt::blue;tracks->viewport()->update();
        write("workflows/command-results/result.json",QJsonDocument(QJsonObject{{"command","edit.inspector.stabilize"},{"success",true},
            {"issued_ms",double(QDateTime::currentMSecsSinceEpoch())}}).toJson());
        QElapsedTimer deadline;deadline.start();while(QFile::exists(files.path()+"/workflows/command-results/result.json") && deadline.elapsed()<1500)QApplication::processEvents();
        TimelineViewportFixture::settle();
        check(displayed()==QColor(Qt::blue),"validated asynchronous workflow completion refreshes Timeline pixels without replaying its command");
        QShortcut immediate(QKeySequence(Qt::Key_F7),&main);immediate.setContext(Qt::ApplicationShortcut);int nativeCalls=0;
        QObject::connect(&immediate,&QShortcut::activated,[&]{++nativeCalls;item->state=Qt::red;tracks->viewport()->update();});
        QKeyEvent nativeKey(QEvent::KeyPress,Qt::Key_F7,Qt::NoModifier);QApplication::sendEvent(fixture.view,&nativeKey);TimelineViewportFixture::settle();
        check(nativeCalls==1 && displayed()==QColor(Qt::red),"a newly created native shortcut needs no discovery delay, key whitelist or replay to update its visible result");
        QMouseEvent down(QEvent::MouseButtonPress,target,physical,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&down);
        QMouseEvent drag(QEvent::MouseMove,target+QPoint(1,0),physical+QPoint(1,0),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(layer,&drag);TimelineViewportFixture::settle();
        check(item->state==Qt::darkGreen && displayed()==QColor(Qt::darkGreen),"native paint-only drag refreshes before release even when no scene item or geometry change is emitted");
        QMouseEvent up(QEvent::MouseButtonRelease,target+QPoint(1,0),physical+QPoint(1,0),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(layer,&up);TimelineViewportFixture::settle();
        const int widgetBefore=widgetCalls;
        QKeyEvent widgetKey(QEvent::KeyPress,Qt::Key_F8,Qt::NoModifier);QApplication::sendEvent(fixture.view,&widgetKey);TimelineViewportFixture::settle();
        check(widgetCalls==widgetBefore+1 && (widgetPoint-QPoint(40,25)).manhattanLength()<3 && displayed()==QColor(Qt::darkBlue),
              "QWidget-owned native shortcuts execute once with queued pointer coordinates and current pixels, without a receiver-class whitelist");
    }
    QWidget pool(&edit);pool.setObjectName("frameEditPageMediaPool");pool.setFocusPolicy(Qt::StrongFocus);pool.show();pool.setFocus();
    const auto scopedCalls=ownCalls;QKeyEvent outside(QEvent::KeyPress,Qt::Key_F6,Qt::NoModifier);QApplication::sendEvent(&pool,&outside);TimelineViewportFixture::settle();
    check(ownCalls==scopedCalls && QApplication::focusWidget()==&pool,"Timeline-only mappings remain available to other panels without firing or stealing their focus");
    QLineEdit editor(fixture.view->viewport());editor.show();editor.setFocus();
    QKeyEvent typed(QEvent::KeyPress,Qt::Key_W,Qt::NoModifier,"w");QApplication::sendEvent(&editor,&typed);TimelineViewportFixture::settle();
    check(editor.text()=="w" && ownCalls==scopedCalls && QApplication::focusWidget()==&editor,"native inline typing remains intact after command observation and coordinate alignment");
    QFile log(qEnvironmentVariable("UNBOUND_DIAGNOSTICS_DIR")+"/events.jsonl");log.open(QIODevice::ReadOnly);
    int presentations=0;bool fresh=true;
    for(const auto &line:log.readAll().split('\n')) {
        const auto record=QJsonDocument::fromJson(line).object();
        if(record["event"]=="timeline.command.presented" && record["native_view"]==QString::number(quintptr(fixture.view),16)) {
            ++presentations;fresh&=record["fresh_native_render"].toBool() && !record["sampled_raster_fingerprint"].toString().isEmpty();
        }
    }
    check(presentations>=15 && fresh,"command diagnostics record fresh native raster revisions after commands rather than only action invocation");
    scale.suspend();
    if(previousPreset.isNull())qunsetenv("OPENGATE_PRESET_DIRECTORY");else qputenv("OPENGATE_PRESET_DIRECTORY",previousPreset);
}
}
