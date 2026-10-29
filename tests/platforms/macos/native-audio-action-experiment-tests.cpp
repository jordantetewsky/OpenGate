// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QGraphicsRectItem>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QSaveFile>
#include <QEventLoop>
#include <cstdio>
#include "../../../src/core/nativeaudioactionexperiment.h"
#include "nativeeditcommands.h"
struct Case {QString id,label;int target;};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir directory;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",directory.path().toUtf8());
    qputenv("UNBOUND_SHORTCUTS_FILE",(directory.path()+"/Shortcuts.json").toUtf8());
    qputenv("UNBOUND_LAUNCH_DIRECTORY",directory.path().toUtf8());
    qputenv("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",directory.path().toUtf8());
    QWidget page;page.setAttribute(Qt::WA_DontShowOnScreen);QVBoxLayout layout(&page);
    QGraphicsScene scene;QGraphicsView view(&scene);view.setObjectName("m_TiTimelineView");layout.addWidget(&view);
    QGraphicsRectItem item(0,0,160,40),other(0,60,160,40);
    for(auto clip:{&item,&other}){clip->setFlag(QGraphicsItem::ItemIsSelectable);scene.addItem(clip);}item.setSelected(true);
    page.resize(500,300);page.show();QApplication::setActiveWindow(&page);view.setFocus();app.processEvents();
    int failures=0;
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
    auto pump=[&]{QEventLoop loop;QTimer::singleShot(40,&loop,&QEventLoop::quit);loop.exec();};
    QList<Case> cases{{"edit.audio.previous-channel-all-source","Previous Audio Channel — All Source Channels",2},
        {"edit.audio.next-channel-all-source","Next Audio Channel — All Source Channels",4},
        {"edit.audio.previous-channel","Previous Audio Channel — Skip Excluded Channels",5},
        {"edit.audio.next-channel","Next Audio Channel — Skip Excluded Channels",4}};
    int caseNumber=0;
    for(const auto &test:cases) {
        const auto permitPath=directory.path()+"/permit-"+QString::number(++caseNumber)+".json";
        auto savePermit=[&](QString clip="clip",QString timeline="timeline",QString project="project") {
            QSaveFile permit(permitPath);permit.open(QIODevice::WriteOnly);
            permit.write(QJsonDocument(QJsonObject{{"schema",1},{"max_invocations",1},{"project_id",project},
                {"timeline_id",timeline},{"item_id",clip},{"expires_ms",double(QDateTime::currentMSecsSinceEpoch()+60000)}}).toJson());permit.commit();
        };savePermit();
        Unbound::NativeAudioActionExperiment experiment(&page,permitPath);
        QMenu root;root.setAttribute(Qt::WA_DontShowOnScreen);auto channels=root.addMenu("Audio Channel");
        channels->setAttribute(Qt::WA_DontShowOnScreen);int nativeCalls=0,current=3,plans=0;
        for(int n=1;n<=5;++n) {
            auto action=channels->addAction(QString::number(n)+" - Source");action->setCheckable(true);action->setChecked(n==3);
            QObject::connect(action,&QAction::triggered,&page,[&,n]{++nativeCalls;current=n;});
        }
        const auto nativeLabel="OpenGate - "+test.label;
        QFile script(directory.path()+"/"+nativeLabel+".py");script.open(QIODevice::WriteOnly);script.write("# opengate-command-route: native-audio-plan-v3");script.close();
        auto backend=new QAction(nativeLabel,&page);backend->setObjectName("workspaceWIPlugins_fixture");page.addAction(backend);
        QObject::connect(backend,&QAction::triggered,&page,[&]{
            ++plans;QFile job(directory.path()+"/audio-channel-plan-job.json");job.open(QIODevice::ReadOnly);
            const auto request=QJsonDocument::fromJson(job.readAll()).object();
            check(request["command"].toString()==test.id,"registered direction and scope select the matching planner");
            QJsonObject plan{{"protocol","native-audio-plan-v3"},{"success",true},{"phase","plan-only"},{"project_id","project"},{"timeline_id","timeline"},
                {"audio_selected",1},{"video_selected",0},{"direction",test.id.contains("previous")?-1:1},
                {"channel_scope",test.id.endsWith("-all-source")?"all-source":"skip-excluded"},{"token",request["token"]},{"issued_ms",double(QDateTime::currentMSecsSinceEpoch())},
                {"items",QJsonArray{QJsonObject{{"status","ready"},{"item_id","clip"},{"before",3},{"after",test.target},{"source_channels",5}}}}};
            QSaveFile result(directory.path()+"/audio-channel-plans/"+request["token"].toString()+".json");
            result.open(QIODevice::WriteOnly);result.write(QJsonDocument(plan).toJson());result.commit();
        });
        Unbound::NativeEditCommands commands(&page,[&]{return &page;});commands.audioExperiment=[&](int direction,bool skip){return experiment.invoke(direction,skip);};
        Unbound::PanelCommands registry(&page);registry.context=[] {return true;};commands.addTo(&registry);
        auto prepare=[&](QGraphicsItem *clicked) {
            QApplication::setActiveWindow(&page);view.setFocus();app.processEvents();
            const auto local=view.mapFromScene(clicked->mapToScene(clicked->boundingRect().center()));
            QContextMenuEvent event(QContextMenuEvent::Mouse,local,view.viewport()->mapToGlobal(local));experiment.observe(view.viewport(),&event);
            QEvent show(QEvent::Show);experiment.observe(&root,&show);experiment.observe(channels,&show);
        };
        auto close=[&]{QMetaObject::invokeMethod(channels,"aboutToHide");QMetaObject::invokeMethod(&root,"aboutToHide");};
        prepare(&other);close();check(!experiment.invoke(1,false),"a different right-click target cannot authorize the selected clip");
        prepare(&item);close();other.setSelected(true);
        check(!experiment.invoke(1,false),"selection change invalidates the prepared context");other.setSelected(false);
        prepare(&item);close();QEvent deactivated(QEvent::ApplicationDeactivate);experiment.observe(qApp,&deactivated);
        check(!experiment.invoke(1,false),"application deactivation invalidates the prepared context");
        prepare(&item);check(!experiment.invoke(1,false),"both native menu hide signals required before invocation");close();
        savePermit("clip","timeline","wrong-project");check(registry.invoke(test.id,"fixture"),"wrong-project request reaches permit validation");pump();
        check(nativeCalls==0 && !QFile::exists(permitPath+".used"),"a working-project mismatch cannot invoke the native action");
        savePermit("clip","wrong-timeline");check(registry.invoke(test.id,"fixture"),"wrong-timeline request reaches permit validation");pump();
        check(nativeCalls==0 && !QFile::exists(permitPath+".used"),"a different Timeline cannot invoke the native action");
        savePermit("wrong-clip");check(registry.invoke(test.id,"fixture"),"registered test reaches the guarded operation");pump();
        check(nativeCalls==0 && !QFile::exists(permitPath+".used"),"mismatched disposable identity performs no native action");
        savePermit();prepare(&item);close();auto focus=QApplication::focusWidget();const auto selected=scene.selectedItems();
        const int plansBefore=plans;
        check(registry.invoke(test.id,"fixture"),"one-shot registered request accepted");pump();
        QFile completed(directory.path()+"/audio-channel-plan-job.json");completed.open(QIODevice::ReadOnly);
        check(QJsonDocument::fromJson(completed.readAll()).object()["inactive"].toBool(),"consumed planning ticket cannot become a late retry");
        check(nativeCalls==1 && current==test.target && plans==plansBefore+1,"planned native QAction triggered exactly once");
        check(!root.isVisible() && !channels->isVisible() && QApplication::focusWidget()==focus && scene.selectedItems()==selected,"no menu presentation focus movement or selection change");
        prepare(&item);close();check(!experiment.invoke(1,false) && nativeCalls==1,"consumed permit prevents replay even after new preparation");
        page.removeAction(backend);delete backend;
    }
    scene.removeItem(&item);scene.removeItem(&other);layout.removeWidget(&view);
    std::printf("native audio action experiment failures=%d\n",failures);return failures?1:0;
}
