#include "native-fixture-start.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QPainter>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelcontrols.h"
#include "latencystats.h"
#include "eventworkprobe.h"

// A paintable proxy, not Resolve's GPU viewer. Time only a paint whose owner has
// reached the requested expansion state; an earlier temporary-host paint is ignored.
class ViewerPaint : public QWidget {
public:
    using QWidget::QWidget;
    std::function<void()> painted;
    void paintEvent(QPaintEvent *) override {
        { QPainter p(this);p.fillRect(rect(),QColor(25,27,30));p.setPen(Qt::gray);p.drawText(rect(),Qt::AlignCenter,"Synthetic viewer; no GPU playback"); }
        if (painted) painted();
    }
};
static void settle(int milliseconds=40) {
    QElapsedTimer timer;timer.start();
    while (timer.elapsed()<milliseconds) { QApplication::processEvents();QThread::usleep(500); }
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv) {
    EventWorkApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files;int failures=0;
    auto check=[&](bool value,const char *message) { if (!value) { ++failures;std::fprintf(stderr,"FAIL: %s\n",message); } };
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    QJsonObject results;
    for (const auto &placement:{QString("standalone"),QString("split"),QString("tabs")}) {
        const auto path=files.path()+"/"+placement;QDir().mkpath(path);
        qputenv("UNBOUND_WORKSPACE_FILE",(path+"/Workspace.json").toUtf8());
        qputenv("UNBOUND_ARRANGEMENTS_FILE",(path+"/Arrangements.json").toUtf8());
        qputenv("UNBOUND_SHORTCUTS_FILE",(path+"/Shortcuts.json").toUtf8());
        Unbound::WorkspaceSettings settings;settings.data["native_workspace_mode"]="native";settings.save();
        QWidget main;main.setObjectName("UiMainWindow");main.resize(1000,700);
        auto outer=new QVBoxLayout(&main);auto page=new QWidget(&main);page->setObjectName("WStackPage_Conform");outer->addWidget(page);
        auto grid=new QGridLayout(page);QList<QWidget *> panes;
        const QStringList names{"widgetEditInspector","frameConformTimelines","widgetEditSourcePlayer","widgetEditTimelinePlayer","frameEditPageMediaPool"};
        ViewerPaint *pixels=nullptr;
        for (int i=0;i<names.size();++i) {
            auto pane=new QWidget(page);pane->setObjectName(names[i]);auto layout=new QVBoxLayout(pane);
            if (i==3) { pixels=new ViewerPaint(pane);pixels->setObjectName("fixtureViewerPixels");layout->addWidget(pixels); }
            else layout->addWidget(new QLabel(names[i],pane));
            grid->addWidget(pane,i/3,i%3);panes.append(pane);
        }
        main.show();settle();
        {
            prepareNativeFixture();PanelControls controls;controls.show();settle();
            auto click=[&](const QString &name) {
                auto button=controls.findChild<QPushButton *>(name);check(button,"fixture command exists");if (button) button->click();
            };
            click("undock_widgetEditTimelinePlayer");click("undock_widgetEditSourcePlayer");settle(160);
            check(panes[3]->window()!=&main && panes[2]->window()!=&main,"viewer sessions detach before measurement");
            if (placement!="standalone") { click(placement=="split" ? "group_0" : "group_3");settle(160); }
            auto original=panes[3]->window();original->setGeometry(100,120,800,480);settle(120);
            if (placement=="tabs") for (auto tabs:original->findChildren<QTabWidget *>()) {
                for (int i=0;i<tabs->count();++i) if (tabs->widget(i)->isAncestorOf(panes[3])) tabs->setCurrentIndex(i);
            }
            settle(100);const auto frame=original->geometry();
            const auto tree=placement=="standalone" ? QJsonObject() : Unbound::panelLayoutSnapshot(original->layout()->itemAt(0)->widget());
            auto toggle=controls.findChild<QPushButton *>("expand_widgetEditTimelinePlayer");check(toggle,"maximize command exists");
            if (!toggle) return 1;
            Unbound::LatencyStats expandDispatch,restoreDispatch,expandReady,restoreReady,expandPaint,restorePaint;
            QJsonObject cold;int timeouts=0;
            app.beginProbe();app.setProbeEnabled(false);
            for (int iteration=0;iteration<21;++iteration) for (bool expanding:{true,false}) {
                if (expanding) { Unbound::activateNativeWindowForTest(original);settle(10); }
                QElapsedTimer timer;bool painted=false; qint64 firstPaint=-1,ready=-1;
                auto stateReady=[&] {
                    auto host=panes[3]->window();
                    if(!expanding)return host==original && !host->property("unboundExpanded").toBool() && host->geometry()==frame;
                    if(!host->property("unboundExpanded").toBool())return false;
                    auto owner=placement=="standalone"?&main:original;
                    const auto margins=host->windowHandle()?host->windowHandle()->frameMargins():QMargins();
                    const auto viewport=Unbound::workspaceViewportBounds(owner,owner==&main?page:nullptr);
                    return host->geometry()==viewport.adjusted(margins.left(),margins.top(),-margins.right(),-margins.bottom());
                };
                pixels->painted=[&] { if (!painted && stateReady()) { painted=true;firstPaint=timer.nsecsElapsed(); } };
                Unbound::logEvent("benchmark.viewer-expansion",{{"placement",placement},{"iteration",iteration},{"expanding",expanding}});
                const auto saveCount=[&] {
                    Unbound::flushDiagnostics();
                    QFile events(files.path()+"/events.jsonl");events.open(QIODevice::ReadOnly);
                    int count=0;
                    while(!events.atEnd()) {
                        const auto event=QJsonDocument::fromJson(events.readLine()).object();
                        if(event["event"]=="ui.work.begin" && event["operation"]=="workspace.save")++count;
                    }
                    return count;
                };
                const auto savesBefore=saveCount();
                app.setProbeEnabled(iteration>0);timer.start();toggle->click();const auto dispatch=timer.nsecsElapsed();
                check(saveCount()==savesBefore,"maximize/restore dispatch does not synchronously persist or refresh workspace menus");
                if (stateReady()) ready=timer.nsecsElapsed();
                while (timer.elapsed()<1500 && (ready<0 || !painted)) {
                    QApplication::processEvents(QEventLoop::AllEvents,5);
                    if (ready<0 && stateReady()) ready=timer.nsecsElapsed();
                    if (!painted) QThread::usleep(250);
                }
                app.setProbeEnabled(false);pixels->painted={};
                if (ready<0 || !painted) ++timeouts;
                if (iteration==0) cold[expanding ? "expand" : "restore"]=QJsonObject{{"dispatch_ms",dispatch/1e6},{"ready_ms",ready/1e6},{"first_proxy_paint_ms",firstPaint/1e6}};
                else {
                    (expanding ? expandDispatch : restoreDispatch).add(dispatch);
                    if (ready>=0) (expanding ? expandReady : restoreReady).add(ready);
                    if (firstPaint>=0) (expanding ? expandPaint : restorePaint).add(firstPaint);
                }
                check(stateReady(),"every toggle reaches the expected host, expansion state and restored frame");settle(15);
                if (!expanding && !tree.isEmpty())
                    check(Unbound::panelLayoutSnapshot(original->layout()->itemAt(0)->widget())==tree,"restore preserves exact split sizes and selected tab");
                // A changed underlying workspace must still take the full
                // restoration path. Keep this outside the warm timing sample.
                if (iteration==0 && expanding && placement!="standalone") original->move(original->pos()+QPoint(20,15));
            }
            results[placement]=QJsonObject{{"cold",cold},{"expand_dispatch",expandDispatch.summary()},{"restore_dispatch",restoreDispatch.summary()},
                {"expand_ready",expandReady.summary()},{"restore_ready",restoreReady.summary()},
                {"expand_first_proxy_paint",expandPaint.summary()},{"restore_first_proxy_paint",restorePaint.summary()},
                {"timeouts",timeouts},{"event_work",app.endProbe()}};
            check(timeouts==0,"all maximize/restore requests produce an observed proxy paint");
            std::printf("%s: expand p50/p95 %.2f/%.2f ms; restore %.2f/%.2f ms (first Qt proxy paint); timeouts=%d\n",qPrintable(placement),
                expandPaint.summary()["p50_ms"].toDouble(),expandPaint.summary()["p95_ms"].toDouble(),
                restorePaint.summary()["p50_ms"].toDouble(),restorePaint.summary()["p95_ms"].toDouble(),timeouts);
        }
        settle();
    }
    Unbound::flushDiagnostics();
    QJsonObject report{{"scope","Synthetic native-window fixture; command callback to Qt proxy paint, not physical key or GPU presentation"},{"qt_version",qVersion()},
        {"samples_per_direction",20},{"failures",failures},{"placements",results}};
    const auto output=qEnvironmentVariable("UNBOUND_VIEWER_LATENCY_REPORT");
    if (!output.isEmpty()) { QSaveFile file(output);check(file.open(QIODevice::WriteOnly),"open benchmark report");file.write(QJsonDocument(report).toJson());check(file.commit(),"save benchmark report"); }
    return failures ? 1 : 0;
}
