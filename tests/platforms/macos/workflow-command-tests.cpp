// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <QProcess>
#include <QLineEdit>
#include <cstdio>
#include "workflowcommandbridge.h"
#include "workflowcommandresults.h"
#include "workflowcommandmenu.h"
#include "panelcommands.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    int accepted=0,attempts=0;
    Unbound::WorkflowCommandBridge bridge(nullptr,files.path(),[&](QString id) { ++attempts; if (id!="panel.toggle.source") return false; ++accepted; return true; });
    auto request=[&](const QString &file,const QByteArray &bytes) { QSaveFile out(files.path()+"/command-requests/"+file); out.open(QIODevice::WriteOnly); out.write(bytes); out.commit(); Unbound::postNativeCommandRequestForTest(); };
    auto settle=[] { QElapsedTimer timer; timer.start(); while (timer.elapsed()<250) { QApplication::processEvents(); QThread::msleep(1); } };
    const auto json=[&](QString id,qint64 time) { return QJsonDocument(QJsonObject{{"command",id},{"issued_ms",double(time)}}).toJson(); };
    request("one.json",json("panel.toggle.source",QDateTime::currentMSecsSinceEpoch())); settle();
    check(accepted==1 && attempts==1,"native workflow request dispatches a whitelisted command on GUI thread");
    settle(); check(accepted==1,"watcher notifications cannot replay a consumed command");
    request("stale.json",json("panel.toggle.source",QDateTime::currentMSecsSinceEpoch()-20000));
    request("future.json",json("panel.toggle.source",QDateTime::currentMSecsSinceEpoch()+20000));
    request("invalid.json","not JSON"); request("oversize.json",QByteArray(5000,'x')); settle();
    check(accepted==1 && attempts==1,"stale future malformed and oversized commands cannot dispatch");
    request("unknown.json",json("arbitrary.code",QDateTime::currentMSecsSinceEpoch())); settle();
    check(attempts==2 && accepted==1,"unknown command IDs cannot bypass registry whitelist");
    check(QDir(files.path()+"/command-requests").entryList({"*.json"},QDir::Files).isEmpty(),"all requests are consumed once including rejected requests");
    {
        Unbound::WorkflowCommandResults results(nullptr,files.path());
        const auto reportPath=files.path()+"/command-results/stabilize.json";
        auto publish=[&](QJsonObject value) {QSaveFile out(reportPath);out.open(QIODevice::WriteOnly);out.write(QJsonDocument(value).toJson());out.commit();
            // Directory notifications may coalesce beyond one fixed 250ms turn.
            // Wait for consumption, preserving a bounded failure if it never arrives.
            QElapsedTimer deadline;deadline.start();while(QFile::exists(reportPath)&&deadline.elapsed()<2000){QApplication::processEvents();QThread::msleep(1);}
            settle();};
        const auto record=QJsonObject{{"command","edit.inspector.stabilize"},{"success",true},
            {"issued_ms",double(QDateTime::currentMSecsSinceEpoch())},{"method","timeline-item-api"},{"attempted",2},{"completed",2}};
        auto logged=[&] {QFile file(files.path()+"/events.jsonl");file.open(QIODevice::ReadOnly);return file.readAll().count("selected-timeline-api-returned");};
        publish(record);
        check(logged()==1&&!QFile::exists(reportPath),"API completion is logged and consumed once");
        settle();check(logged()==1&&attempts==2,"completion notifications cannot replay or dispatch a command");
        auto failure=record;failure["success"]=false;failure["completed"]=0;publish(failure);
        check(logged()==2,"actual API failure also produces completion evidence");
        auto invalid=record;invalid["command"]="arbitrary.code";publish(invalid);
        check(logged()==2&&attempts==2,"result files cannot execute arbitrary commands");
        auto audio=record;audio["command"]="edit.audio.next-channel";audio["changed"]=2;audio["skipped"]=1;
        publish(audio);
        QFile evidence(files.path()+"/events.jsonl");evidence.open(QIODevice::ReadOnly);const auto bytes=evidence.readAll();
        check(bytes.count("command.audio-channel.result")==1&&attempts==2,"audio completion is consumed without replay and records its own result");
    }
    {
        QWidget popup(nullptr,Qt::Popup);popup.setAttribute(Qt::WA_DontShowOnScreen);popup.show();settle();
        check(QApplication::activePopupWidget()==&popup,"fixture owns a tracked popup");
        request("menu.json",json("panel.toggle.source",QDateTime::currentMSecsSinceEpoch()));settle();
        check(accepted==1 && attempts==2,"workflow request is deferred while menu tracking remains active");
        check(QFile::exists(files.path()+"/command-requests/menu.json"),"deferred request is not lost or claimed prematurely");
        popup.hide();settle();check(accepted==2&&attempts==3,"menu close dispatches retained request exactly once");
        settle();check(accepted==2,"menu and filesystem wakeups cannot replay a deferred request");
    }
    {
        const auto directory=files.path()+"/assigned-key";qputenv("UNBOUND_LAUNCH_DIRECTORY",directory.toUtf8());
        qputenv("UNBOUND_SHORTCUTS_FILE",(directory+"/Shortcuts.json").toUtf8());
        QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);
        QMenu native,root(&main);
        auto action=native.addAction("OpenGate - Show-Hide Source Monitor");action->setObjectName("workspaceWIPlugins_fixture");
        // Menu activation tests the wrapper bridge; it does not prove native key dispatch.
        int nativeCalls=0,registryCalls=0;
        const auto wrapper=QFileInfo(QStringLiteral(__FILE__)).dir().absoluteFilePath("../../../build/macos/workflow-commands/OpenGate - Show-Hide Source Monitor.py");
        QObject::connect(action,&QAction::triggered,[&] {
            ++nativeCalls;check(QProcess::execute("python3",{wrapper})==0,"native workflow handler runs the actual generated command script");
        });
        Unbound::PanelCommands commands(nullptr);commands.context=[&] { return QApplication::activeWindow()==&main; };
        commands.add({"panel.toggle.widgetEditSourcePlayer","Show / Hide Source Monitor",[] { return true; },[&] { ++registryCalls; }});
        Unbound::WorkflowCommandBridge routed(nullptr,directory,[&](QString id) { return commands.invoke(id,"assigned-key-fixture"); });
        Unbound::WorkflowCommandMenu presentation(nullptr);presentation.wire(&root);
        main.show();QApplication::setActiveWindow(&main);settle();
        action->trigger();settle();
        check(nativeCalls==1 && registryCalls==1,"menu activation reaches generated Python request and command registry exactly once");
        settle();check(registryCalls==1,"native notification and filesystem fallback cannot double execute an assigned key");
        QLineEdit typing(&main);typing.show();typing.setFocus();settle();
        check(QApplication::focusWidget()==&typing,"typing guard fixture owns keyboard focus");
        action->trigger();settle();
        check(registryCalls==1,"native-assigned OpenGate command cannot run while editing text");
    }
    std::printf("workflow command failures=%d\n",failures); return failures ? 1 : 0;
}
