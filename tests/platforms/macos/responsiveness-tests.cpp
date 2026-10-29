#include "native-fixture-start.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QThread>
#include <QTreeWidget>
#include <cstdio>
#include "panelcontrols.h"
#include "responsiveness.h"
#include "activationbridge.h"
#include "eventworkprobe.h"

static void settle(int duration = 50)
{
    QElapsedTimer clock; clock.start();
    while (clock.elapsed() < duration) { QApplication::processEvents(); QThread::msleep(1); }
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv)
{
    EventWorkApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files;
    qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());
    qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    Unbound::WorkspaceSettings settings; settings.data["native_workspace_mode"] = "native"; settings.save();
    int failures = 0;
    const auto check = [&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    Unbound::LatencyStats known; for (int i=1;i<=100;++i) known.add(qint64(i)*1000000);
    check(known.summary()["p95_ms"].toDouble() == 95 && known.summary()["over_16_67_ms"].toInt() == 84,"latency percentile and frame-budget accounting");
    Unbound::WindowDiagnostics windows(&app); app.installEventFilter(&windows);
    Unbound::InteractionDiagnostics interactions(&app); Unbound::ResolveActivationBridge activation(&app);
    Unbound::ResponsivenessProbe responsiveness(&app);
    QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100,700);
    auto outer = new QVBoxLayout(&main); auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page);
    auto grid = new QGridLayout(page);
    QStringList names{"widgetEditInspector","frameConformTimelines","widgetEditSourcePlayer","widgetEditTimelinePlayer","frameEditPageMediaPool"};
    for (int i=0;i<names.size();++i) {
        auto pane = new QWidget(page); pane->setObjectName(names[i]); auto layout = new QVBoxLayout(pane);
        auto tree = new QTreeWidget(pane); tree->setColumnCount(2); tree->setHeaderLabels({"Native-style content","Value"});
        for (int row=0;row<300;++row) new QTreeWidgetItem(tree,{QString("Item %1").arg(row),QString::number(row)});
        layout->addWidget(tree); auto button = new QPushButton("Native control",pane); button->setObjectName("fixtureNativeControl"); layout->addWidget(button);
        grid->addWidget(pane,i/3,i%3);
    }
    QAction viewer("Single Viewer Mode",&main); viewer.setObjectName("workspaceSingleViewerModeToggle"); viewer.setCheckable(true);
    main.show(); settle(); prepareNativeFixture();PanelControls controls; controls.show(); settle();
    for (const auto &name : names) { controls.findChild<QPushButton *>("undock_"+name)->click(); settle(130); }
    const auto host = [](const QString &name) -> Unbound::MacFloatingHost * {
        for (auto window : QApplication::topLevelWidgets()) if (window->objectName() == "unboundFloating_"+name)
            return static_cast<Unbound::MacFloatingHost *>(window);
        return nullptr;
    };
    auto moving = host("frameConformTimelines"); auto target = host("widgetEditTimelinePlayer");
    check(moving && target,"benchmark retains real floating hosts and full controller behavior");
    if (!moving || !target) return 1;
    for (int i=0;i<names.size();++i) if (auto window = host(names[i])) window->setGeometry(30+i*35,80+i*30,360,260);
    moving->setGeometry(30,80,360,260); target->setGeometry(500,200,400,320);
    controls.move(1100,700); moving->raise(); settle(150);
    QJsonObject scenarios;
    const auto measure = [&](const QString &name,int count,const std::function<void(int)> &operation,
                             const std::function<void(int)> &prepare = {}) {
        Unbound::LatencyStats timing, dispatch, deferred;
        app.beginProbe();
        for (int i=0;i<count;++i) {
            app.setProbeEnabled(false);
            if (prepare) prepare(i);
            app.setProbeEnabled(true);
            QElapsedTimer turn; turn.start(); operation(i);
            const auto dispatched = turn.nsecsElapsed();
            QApplication::processEvents(QEventLoop::AllEvents,10);
            const auto completed = turn.nsecsElapsed();
            dispatch.add(dispatched); deferred.add(completed-dispatched); timing.add(completed);
            app.setProbeEnabled(false);
            while (turn.elapsed() < 16) { QApplication::processEvents(); QThread::msleep(1); }
        }
        auto summary = timing.summary();
        summary["event_work"] = app.endProbe();
        summary["dispatch"] = dispatch.summary(); summary["deferred_event_drain"] = deferred.summary();
        scenarios[name] = summary;
        std::printf("%s p95=%.3f ms max=%.3f ms dispatch_p95=%.3f ms deferred_p95=%.3f ms\n",qPrintable(name),
            summary["p95_ms"].toDouble(),summary["max_ms"].toDouble(),
            dispatch.summary()["p95_ms"].toDouble(),deferred.summary()["p95_ms"].toDouble());
    };
    Unbound::activateNativeWindowForTest(moving); settle(80);
    check(Unbound::nativeKeyWindowNumber()==Unbound::nativeWindowNumber(moving),
        "scripted free drag starts in the native key window like an actual pointer press");
    QPoint press = moving->mapToGlobal(QPoint(100,8));
    const QPoint grabOffset = press-moving->geometry().topLeft(); app.setScriptedPointer(true); moving->beginPointerDrag(press,false);
    int trackingError = 0;
    const auto available = moving->screen()->availableGeometry();
    const QPoint freePoint(available.right()-80,available.bottom()-80);
    measure("free_drag_event_turn",120,[&](int i) {
        const auto point = freePoint+QPoint(i%20,0);
        QMouseEvent event(QEvent::MouseMove,QPointF(100,8),QPointF(point),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(moving,&event);
        const auto error = (moving->geometry().topLeft()-(point-grabOffset)).manhattanLength();
        if (error && !trackingError)
            std::fprintf(stderr,"Tracking offset=%d,%d pointer=%d,%d actual=%d,%d frame=%d,%d error=%d\n",grabOffset.x(),grabOffset.y(),point.x(),point.y(),moving->x(),moving->y(),moving->frameGeometry().x(),moving->frameGeometry().y(),error);
        trackingError = qMax(trackingError,error);
    });
    const QPoint released = freePoint+QPoint(19,0);
    QMouseEvent release(QEvent::MouseButtonRelease,QPointF(100,8),QPointF(released),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(moving,&release); app.setScriptedPointer(false); settle();
    check(trackingError == 0 && moving->geometry().topLeft() == released-grabOffset,
        "every free-drag frame preserves the actual native grab offset without trailing or forced docking");
    moving->setGeometry(30,80,360,260); Unbound::activateNativeWindowForTest(moving); settle(80);
    check(Unbound::nativeKeyWindowNumber()==Unbound::nativeWindowNumber(moving),"dock approach establishes native key-window context");
    press = moving->mapToGlobal(QPoint(100,8)); app.setScriptedPointer(true); moving->beginPointerDrag(press,false);
    const auto edge = target->frameGeometry().right();
    measure("dock_approach_event_turn",120,[&](int i) {
        const auto point = QPoint(edge-(i%12),target->frameGeometry().center().y());
        QMouseEvent event(QEvent::MouseMove,QPointF(100,8),QPointF(point),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(moving,&event);
    });
    // Release away from targets; target computation remains fully enabled throughout.
    QMouseEvent away(QEvent::MouseMove,QPointF(100,8),QPointF(freePoint),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(moving,&away); QApplication::sendEvent(moving,&release); app.setScriptedPointer(false); settle();
    auto timeline = main.findChild<QWidget *>("frameConformTimelines"); auto program = main.findChild<QWidget *>("widgetEditTimelinePlayer");
    QPoint commitPoint; QRect commitFrame;
    measure("dock_commit_event_turn",32,[&](int) {
        QMouseEvent up(QEvent::MouseButtonRelease,QPointF(100,8),QPointF(commitPoint),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(moving,&up); app.setScriptedPointer(false);
    },[&](int iteration) {
        if (iteration) {
            check(timeline->window() == program->window() && program->window()->frameGeometry() == commitFrame,
                "every repeated drop commits the predicted group and frame");
            for (auto button : timeline->window()->findChildren<QPushButton *>("unboundDetachPanel")) {
                if (button->parentWidget()->parentWidget()->isAncestorOf(timeline)) { button->click(); break; }
            }
            settle();
            check(timeline->window() != program->window(),"each repeated drop begins with two independently floating panels");
        }
        target = host("widgetEditTimelinePlayer"); moving = host("frameConformTimelines");
        check(target && moving,"each tear-off restores independently owned floating hosts");
        if (!target || !moving) return;
        target->setGeometry(500,200,760,400); moving->setGeometry(30,80,360,260);
        target->raise(); moving->raise(); settle(300);
        Unbound::activateNativeWindowForTest(moving); settle(20);
        press = moving->mapToGlobal(QPoint(100,8)); app.setScriptedPointer(true); moving->beginPointerDrag(press,false);
        commitPoint = QPoint(target->frameGeometry().right()-4,target->frameGeometry().center().y());
        commitFrame = target->frameGeometry();
        QMouseEvent approach(QEvent::MouseMove,QPointF(100,8),QPointF(commitPoint),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(moving,&approach);
    }); settle(120);
    check(timeline->window() == program->window(),"real release commits the proposed split with docking enabled");
    check(program->window()->frameGeometry() == commitFrame,"committed drop retains the destination frame predicted by the preview");
    controls.findChild<QComboBox *>("groupFirst")->setCurrentIndex(2); controls.findChild<QComboBox *>("groupSecond")->setCurrentIndex(3);
    controls.findChild<QPushButton *>("group_0")->click(); settle(220);
    auto source = main.findChild<QWidget *>("widgetEditSourcePlayer");
    check(source->window() == program->window(),"benchmark preserves linked Source and Program in the nested group");
    controls.findChild<QComboBox *>("groupFirst")->setCurrentIndex(0); controls.findChild<QComboBox *>("groupSecond")->setCurrentIndex(4);
    controls.findChild<QPushButton *>("group_0")->click(); settle(250);
    auto inspector = main.findChild<QWidget *>("widgetEditInspector"); const auto frame = inspector->window()->geometry();
    auto splitter = inspector->window()->findChild<QSplitter *>();
    check(splitter && splitter->count() >= 2,"benchmark exposes a real shared divider");
    if (splitter) {
        auto handle = splitter->handle(1); const QPoint local = handle->rect().center(), global = handle->mapToGlobal(local);
        QMouseEvent down(QEvent::MouseButtonPress,QPointF(local),QPointF(global),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(handle,&down);
        measure("divider_resize_event_turn",120,[&](int i) {
            const auto point = global+QPoint((i%40)-20,0);
            QMouseEvent move(QEvent::MouseMove,QPointF(handle->mapFromGlobal(point)),QPointF(point),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(handle,&move);
        });
        QMouseEvent up(QEvent::MouseButtonRelease,QPointF(local),QPointF(global),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(handle,&up);
        check(inspector->window()->geometry() == frame,"divider stress preserves outer group bounds");
    }
    measure("door_toggle_event_turn",40,[&](int i) {
        controls.findChild<QPushButton *>((i%2 ? "show_" : "hide_")+QString("widgetEditInspector"))->click();
    });
    check(inspector->window()->geometry() == frame && inspector->isVisible(),"door stress preserves the magnetic outer frame and final visibility");
    const auto viewerFrame = source->window()->geometry();
    measure("viewer_command_event_turn",40,[&](int) { viewer.trigger(); });
    check(!viewer.isChecked() && source->isVisible() && program->isVisible() && source->window()->geometry() == viewerFrame,
        "viewer command completes every toggle pair without losing panels or resizing the group");
    auto control = inspector->findChild<QPushButton *>("fixtureNativeControl");
    measure("button_dispatch_event_turn",40,[&](int) { control->click(); });
    responsiveness.flush(); windows.flushGeometry();
    Unbound::flushDiagnostics();
    QJsonArray trace;
    QFile events(files.path()+"/events.jsonl"); events.open(QIODevice::ReadOnly);
    while (!events.atEnd()) {
        const auto record = QJsonDocument::fromJson(events.readLine()).object();
        if (record["event"] == "ui.work" || record["event"] == "drag.timing" || record["event"] == "ui.responsiveness" ||
            record["event"] == "group.reserve.take" || record["event"] == "drag.geometry.mismatch" || record["event"] == "drag.geometry.summary" || record["event"] == "drag.begin" || record["event"] == "drag.end" || record["event"] == "drag.capture.recovered") trace.append(record);
    }
    bool freeReleased=false;
    for (const auto &entry:trace) {
        const auto record=entry.toObject();
        if (record["event"]=="drag.end" && record["window"]=="unboundFloating_frameConformTimelines") {
            freeReleased=true;
            check(!record["cancelled"].toBool() && record["reason"]=="qt-mouse-up",
                "scripted free drag remains active until its explicit release"); break;
        }
    }
    check(freeReleased,"scripted free drag records an explicit completed release");
    const QString reportPath = qEnvironmentVariable("UNBOUND_RESPONSIVENESS_REPORT",QCoreApplication::applicationDirPath()+"/responsiveness-report.json");
    const int functionalFailures = failures;
    for (auto item = scenarios.begin();item != scenarios.end();++item) {
        check(item.value().toObject()["p95_ms"].toDouble() < 16.667,"p95 event turn stays within one 60 Hz frame budget");
        check(item.value().toObject()["max_ms"].toDouble() < 100,"no event turn stalls for 100 ms");
    }
    const int budgetFailures = failures-functionalFailures;
    QJsonArray displays;
    for (auto screen:QGuiApplication::screens()) {
        const auto geometry=screen->geometry();
        displays.append(QJsonObject{{"x",geometry.x()},{"y",geometry.y()},{"width",geometry.width()},
            {"height",geometry.height()},{"dpr",screen->devicePixelRatio()},{"refresh_hz",screen->refreshRate()}});
    }
    QSaveFile report(reportPath); check(report.open(QIODevice::WriteOnly),"open responsiveness artifact");
    report.write(QJsonDocument(QJsonObject{{"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"scope","Real Qt/native windows with simulated input; not Resolve playback or GPU presentation"},
        {"scripted_pointer_isolation",true},{"ignored_unsolicited_pointer_moves",app.ignoredPointerMoves()},{"features_enabled",true},{"functional_failures",functionalFailures},{"budget_failures",budgetFailures},
        {"passed",failures == 0},{"displays",displays},{"scenarios",scenarios},{"trace",trace}}).toJson());
    check(report.commit(),"write responsiveness artifact");
    std::printf("Report: %s\n",qPrintable(reportPath));
    return failures ? 1 : 0;
}
