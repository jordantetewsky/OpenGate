// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelcommandmenu.h"
int main(int argc,char **argv){
    QApplication app(argc,argv);QTemporaryDir files;qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;auto check=[&](bool ok,const char *message){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",message);}};
    auto settle=[] {QElapsedTimer timer;timer.start();while(timer.elapsed()<100){QApplication::processEvents();QThread::msleep(1);}};
    QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.show();QApplication::setActiveWindow(&main);settle();
    Unbound::PanelCommands registry(nullptr);registry.context=[] {return true;};
    int stabilizations=0,fixed=0,expanded=0,shown=0,audio=0;bool available=true;
    registry.add({"edit.inspector.stabilize","Edit Inspector / Stabilize",[&]{return available;},[&]{++stabilizations;}});
    registry.add({"edit.timeline.fixed-playhead","Edit Timeline / Toggle Fixed Playhead",[&]{return available;},[&]{++fixed;}});
    registry.add({"panel.expand.source","Maximize / Restore Source Monitor",[&]{return available;},[&]{++expanded;}});
    registry.add({"panel.toggle.source","Show / Hide Source Monitor",[&]{return available;},[&]{++shown;}});
    registry.add({"edit.audio.previous-channel","Previous Audio Channel — Skip Excluded Channels",[&]{return available;},[&]{--audio;}});
    registry.add({"edit.audio.next-channel","Next Audio Channel — Skip Excluded Channels",[&]{return available;},[&]{++audio;}});
    registry.add({"edit.audio.previous-channel-all-source","Previous Audio Channel — All Source Channels",[&]{return available;},[&]{--audio;}});
    registry.add({"edit.audio.next-channel-all-source","Next Audio Channel — All Source Channels",[&]{return available;},[&]{++audio;}});
    QMenu native;auto original=native.addAction("OpenGate - Edit - Stabilize");original->setObjectName("workspaceWIPlugins_0");
    const auto originalActions=native.actions();const auto hosts=original->associatedWidgets();
    QMenu root;QPointer<QMenu> retained;
    {
        Unbound::PanelCommandMenu presentation(nullptr,&registry);auto menu=presentation.menu(&root);retained=menu;root.addMenu(menu);
        check(presentation.menu(&root)==menu&&root.findChildren<QMenu *>("openGateCommands").size()==1,"repeated wiring reuses one owned submenu");
        auto find=[&](QString id){for(auto action:menu->findChildren<QAction *>())if(action->property("openGateCommandId")==id)return action;return static_cast<QAction *>(nullptr);};
        auto stabilize=find("edit.inspector.stabilize");auto fixedAction=find("edit.timeline.fixed-playhead");
        check(stabilize&&fixedAction&&find("panel.expand.source")&&find("panel.toggle.source"),"command IDs are discoverable in presentation groups");
        QWidget popup(&main,Qt::Popup);popup.setAttribute(Qt::WA_DontShowOnScreen);popup.show();settle();
        stabilize->trigger();settle();check(stabilizations==0,"menu tracking cannot execute an operation before closing");
        popup.hide();QApplication::setActiveWindow(&main);settle();check(stabilizations==1,"closed menu delegates to shared registry exactly once");
        settle();check(stabilizations==1,"deferred menu callback cannot replay");
        fixedAction->trigger();find("panel.expand.source")->trigger();find("panel.toggle.source")->trigger();settle();
        check(fixed==1&&expanded==1&&shown==1,"menu uses existing fixed, expansion and visibility command operations");
        auto next=find("edit.audio.next-channel"),previous=find("edit.audio.previous-channel");
        check(next&&previous&&next->text()=="Next Audio Channel — Skip Excluded Channels"&&previous->text()=="Previous Audio Channel — Skip Excluded Channels","audio menu uses exact catalog labels");
        if(next&&previous){next->trigger();settle();check(audio==1,"next menu command dispatches once");previous->trigger();settle();check(audio==0,"previous menu command dispatches once");}
        auto nextAll=find("edit.audio.next-channel-all-source"),previousAll=find("edit.audio.previous-channel-all-source");
        check(nextAll&&previousAll&&nextAll->text()=="Next Audio Channel — All Source Channels"&&previousAll->text()=="Previous Audio Channel — All Source Channels","all-source commands remain separately discoverable");
        if(nextAll&&previousAll){nextAll->trigger();settle();check(audio==1,"next all-source menu dispatches once");previousAll->trigger();settle();check(audio==0,"previous all-source menu dispatches once");}
        available=false;QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection);
        check(!stabilize->isEnabled()&&!fixedAction->isEnabled(),"unavailable page commands are disabled without native action relocation");
        check(native.actions()==originalActions&&original->parent()==&native&&original->associatedWidgets()==hosts,"native workflow ordering, ownership and hosts remain untouched");
    }
    check(!retained&&native.actions()==originalActions,"presentation teardown removes only its own submenu");
    std::printf("panel command menu failures=%d\n",failures);return failures?1:0;
}
