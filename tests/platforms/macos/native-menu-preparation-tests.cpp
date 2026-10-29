// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include "../../../src/core/nativeaudiochannelprobe.h"
#include "nativeeditcommands.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir dir;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",dir.path().toUtf8());
    QWidget page;page.setAttribute(Qt::WA_DontShowOnScreen);page.show();
    int failures=0,calls=0,preparations=0;
    auto check=[&](bool ok,const char *message){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",message);}};
    QMenu clip("Clip");clip.addAction("Clip Attributes...");
    QMenu root(&page);auto channels=root.addMenu("Audio Channel");
    auto action=channels->addAction("1 - Channel Three");action->setCheckable(true);action->setChecked(true);
    QObject::connect(action,&QAction::triggered,[&]{++calls;});
    QObject::connect(&root,&QMenu::aboutToShow,[&]{++preparations;});
    const auto geometry=root.geometry();const auto parent=channels->parent();const auto active=QApplication::activeWindow();
    Unbound::NativeAudioChannelProbe probe(nullptr);
    probe.inspectPreparation(&root);probe.inspectPreparation(&root);probe.inspectPreparation(channels);
    probe.inspectShownMenu(&root);
    check(!calls && !preparations && !root.isVisible() && !channels->isVisible(),"inspection invokes nothing and shows no UI");
    check(root.geometry()==geometry && channels->parent()==parent && QApplication::activeWindow()==active,"ownership geometry and active window preserved");
    // Fixture stimulus only: the production probe must observe, never emit.
    QMetaObject::invokeMethod(&root,"aboutToShow",Qt::DirectConnection);
    QMetaObject::invokeMethod(&root,"aboutToHide",Qt::DirectConnection);
    delete channels;
    Unbound::flushDiagnostics();QFile log(dir.path()+"/events.jsonl");log.open(QIODevice::ReadOnly);const auto data=log.readAll();
    check(data.contains("native-clip-menu") && data.contains("Clip Attributes..."),"native Clip menu connections and action structure inspected");
    check(data.contains("audio.native-menu.connections"),"public Qt connection inspection recorded");
    check(data.count("first-observed")==3,"root child and native Clip menu observed exactly once");
    check(data.contains("about-to-show") && data.contains("about-to-hide") && data.contains("destroyed"),"preparation and destruction lifecycle captured");
    check(data.contains("aboutToShow()") && data.contains("associated_widgets") && data.contains("owners"),"available methods and ownership recorded");
    check(calls==0 && preparations==1,"no native action invocation");
    Unbound::NativeEditCommands commands(nullptr,[&]{return &page;});
    for(int direction:{-1,1})for(bool skip:{false,true})
        check(!commands.audioChannel(direction,skip),"all four audio execution paths explicitly blocked");
    std::printf("native menu preparation failures=%d\n",failures);return failures?1:0;
}
