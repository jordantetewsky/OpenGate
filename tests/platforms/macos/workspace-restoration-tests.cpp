// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "workspacerestoration.h"

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;
    const auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QStringList steps; QString validation,blocked,application,message; bool running=true,pending=false;
    Unbound::WorkspaceRestoration restore(nullptr);
    restore.running=[&] { return running; }; restore.report=[&](QString value) { message=value; };
    restore.validate=[&](const QJsonObject &) { steps.append("validate"); return validation; };
    restore.dock=[&] { steps.append("dock"); return blocked; };
    restore.apply=[&](const QJsonObject &) { steps.append("apply"); return application; };
    restore.resume=[&] { steps.append("resume"); };
    QStringList missing;
    restore.unavailable=[&] { steps.append("unavailable"); return missing; };
    restore.groupsPending=[&] { return pending; };
    const QJsonObject preset{{"id","saved-layout"},{"workspace",QJsonObject{{"version",1}}}};
    restore.preset({},true);
    check(steps.isEmpty() && message.contains("Select"),"missing preset cannot change the current layout");
    restore.preset(preset,false);
    check(steps.isEmpty() && message.contains("Edit page"),"inactive page cannot start a restore");
    validation="Invalid native plan"; restore.preset(preset,true);
    check(steps==QStringList{"validate"} && message==validation,"preflight rejection occurs before any docking");
    steps.clear(); validation.clear(); blocked="widgetEditInspector"; restore.preset(preset,true);
    check(steps==QStringList({"validate","dock"}) && message.contains("safely"),"failed docking cannot apply or resume another workspace");
    steps.clear(); blocked.clear(); application="Native position changed"; restore.preset(preset,true);
    check(steps==QStringList({"validate","dock","apply","dock"}) && message.contains(application),"failed application cannot resume partially applied state");
    steps.clear(); application.clear(); pending=true; restore.preset(preset,true);
    check(steps==QStringList({"validate","dock","apply","resume","unavailable"}) && message=="Restoring arrangement…",
        "valid restore preserves preflight, docking, application and resume ordering");
    steps.clear(); pending=false; missing=QStringList{QStringLiteral("Source Monitor")}; restore.preset(preset,true);
    check(message=="Unavailable: Source Monitor","unavailable panels are reported rather than counted as restored");
    missing.clear(); restore.finishPreset();
    check(message=="Arrangement restored.","success is reported only after required panels and groups complete");
    pending=true;restore.preset(preset,true);restore.cancel();pending=false;restore.finishPreset();
    check(message=="Restoring arrangement…","cancelled restore cannot report later success");
    restore.checkpoint=[] {return QJsonObject{{"page","edit"},{"layout_mode","native"}};};
    pending=true;restore.preset(preset,true);steps.clear();
    check(restore.failPreset("Media Pool unavailable")&&steps==QStringList({"dock","apply","resume","unavailable"})&&message.contains("Could not restore"),"failed acquisition releases partial sessions and resumes the previous checkpoint");
    check(restore.active(),"rollback remains an active transaction while native acquisitions are pending");
    pending=false;restore.finishPreset();
    check(!restore.active()&&message.contains("Previous workspace restored"),"rollback reports completion only after its acquisitions finish");
    check(!restore.failPreset("again"),"completed rollback cannot recurse");
    pending=true;restore.preset(preset,true);restore.failPreset("forward failure");
    check(restore.failPreset("rollback acquisition failed")&&!restore.active()&&message.contains("previous workspace"),"rollback acquisition failure stops without recursively attempting the same restore");
    check(restore.savingBlocked(),"failed rollback cannot overwrite the last complete persisted checkpoint");
    restore.cancel();check(!restore.savingBlocked(),"explicit recovery releases the failed rollback save guard");
    const auto settle=[] {
        QElapsedTimer clock; clock.start();
        while (clock.elapsed()<230) { QCoreApplication::processEvents(); QThread::msleep(1); }
    };
    int old=0,groups=0,background=0;
    restore.settled(true,true,[&] { ++old; },[&] { ++old; });
    restore.settled(true,true,[&] { ++groups; },[&] { ++background; }); settle();
    check(old==0 && groups==1 && background==1,"superseded settle callbacks cannot mutate the current workspace");
    restore.settled(true,true,[&] { ++old; },[&] { ++old; }); restore.cancel(); settle();
    check(old==0,"explicit cancellation invalidates both delayed restore stages");
    running=false; restore.settled(true,true,[&] { ++old; },[&] { ++old; }); settle();
    check(old==0,"shutdown suppresses delayed layout and background writes");
    Unbound::flushDiagnostics(); QFile events(files.path()+"/events.jsonl"); events.open(QIODevice::ReadOnly);
    const auto log=events.readAll();
    check(log.contains("workspace.native-plan.failed") && log.contains("arrangement.restore.failed") && log.contains("arrangement.restore"),
        "preflight failures, blocked panels and restore outcomes remain diagnosable");
    if (!failures) std::puts("PASS: workspace preflight, restore ordering, failure boundaries and stale callback cancellation");
    return failures ? 1 : 0;
}
