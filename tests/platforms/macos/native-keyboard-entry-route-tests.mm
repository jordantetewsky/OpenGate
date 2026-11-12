// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "../../../src/core/nativekeyboardentryroute.h"
#include "../../core/input-customization/nativekeyboardmenuchecks.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir logs;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0;auto check=[&](bool condition,const char *message){
        if(!condition){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}
    };
    QWidget window;window.setAttribute(Qt::WA_DontShowOnScreen);window.show();
    QMenu menu(&window);auto before=menu.addAction("Before");
    auto original=menu.addAction("Keyboard Customization...");
    original->setObjectName("resolveCustomizedActions");
    original->setShortcut(QKeySequence("Alt+Shift+K"));
    auto after=menu.addAction("After");const auto order=menu.actions();
    const auto associations=original->associatedWidgets();
    int nativeCalls=0,panelCalls=0;bool available=true,enabled=true,catalogueOriginal=false;
    QObject::connect(original,&QAction::triggered,[&]{++nativeCalls;});
    QShortcut shortcut(QKeySequence("Alt+Shift+J"),&window);
    QObject::connect(&shortcut,&QShortcut::activated,[&]{++nativeCalls;});
    QStringList owners{original->objectName()};
    auto settle=[] {for(int i=0;i<5;++i)QApplication::processEvents();};
    OpenGateTest::nativeKeyboardMenuChecks(check,settle);
    {
        Unbound::NativeKeyboardEntryRoute route;
        route.enabled=[&]{return enabled;};
        route.openPanel=[&]{++panelCalls;catalogueOriginal=menu.actions()==order;return available;};
        route.shortcutOwners=[&](const QKeySequence &){return owners;};
        auto openMenu=[&]{QMetaObject::invokeMethod(&menu,"aboutToShow",Qt::DirectConnection);};
        openMenu();auto mirror=menu.actions().value(1);
        check(menu.actions().size()==3 && mirror!=original && menu.actions().front()==before && menu.actions().back()==after,"menu lease preserves row position");
        check(mirror->shortcuts().isEmpty() && original->shortcut()==QKeySequence("Alt+Shift+K"),"menu route never registers or rewrites a shortcut");
        mirror->trigger();settle();
        check(panelCalls==1 && nativeCalls==0 && catalogueOriginal && menu.actions()==order,"menu opens panel once after restoring native catalogue row");
        check(original->associatedWidgets()==associations,"menu lease restores original action associations");
        available=false;openMenu();menu.actions().at(1)->trigger();settle();
        check(panelCalls==2 && nativeCalls==1 && menu.actions()==order,"factory failure invokes original native command once without recursion");
        available=true;
        QShortcutEvent actionEvent(original->shortcut(),123,false);QApplication::sendEvent(original,&actionEvent);
        check(panelCalls==3 && nativeCalls==1,"original QAction assigned shortcut routes to panel");
        QShortcutEvent shortcutEvent(shortcut.key(),shortcut.id(),false);QApplication::sendEvent(&shortcut,&shortcutEvent);
        check(panelCalls==4 && nativeCalls==1,"native QShortcut follows current preset owner rather than a hardcoded key");
        owners=QStringList{"differentNativeCommand"};
        QShortcutEvent reassigned(shortcut.key(),shortcut.id(),false);QApplication::sendEvent(&shortcut,&reassigned);
        check(panelCalls==4 && nativeCalls==2,"changed preset owner preserves ordinary native shortcut delivery");
        owners=QStringList{original->objectName()};available=false;
        QShortcutEvent failed(shortcut.key(),shortcut.id(),false);QApplication::sendEvent(&shortcut,&failed);
        check(panelCalls==5 && nativeCalls==3,"factory failure leaves shortcut event to Qt exactly once");
        available=true;enabled=false;openMenu();
        check(menu.actions()==order,"unsupported workspace retains original menu route");
        enabled=true;openMenu();QMetaObject::invokeMethod(&menu,"aboutToHide",Qt::DirectConnection);settle();
        check(menu.actions()==order,"closing without selection restores native row");
        openMenu();
    }
    check(menu.actions()==order && original->associatedWidgets()==associations,"adapter destruction restores an active lease");
    original->trigger();check(nativeCalls==4,"native signal connections survive adapter destruction");
    {
        Unbound::NativeKeyboardEntryRoute route;
        route.openPanel=[&]{++panelCalls;return true;};
        QMetaObject::invokeMethod(&menu,"aboutToShow",Qt::DirectConnection);
        auto mirror=menu.actions().value(1);
        check(mirror!=original,"quit fixture acquires native menu lease");
        mirror->trigger();
        const auto beforePanel=panelCalls,beforeNative=nativeCalls;
        route.abandonForQuit();settle();
        check(panelCalls==beforePanel && nativeCalls==beforeNative,
              "queued menu request cannot open either editor after quit abandonment");
        check(menu.actions().contains(mirror) && !menu.actions().contains(original),
              "quit abandonment does not rewrite native menu ownership");
    }
    check(!menu.actions().contains(original),"quit destruction never reinserts native menu action");
    {
        Unbound::NativeKeyboardEntryRoute route;
        int aliasCalls=0;bool edit=true;
        route.enabled=[&]{return edit;};route.openPanel=[&]{++aliasCalls;return true;};
        QStringList aliasOwners{"FairlightTimeline.trimExtendEditSelectionToPreviousEdit"};
        route.shortcutOwners=[&](const QKeySequence &){return aliasOwners;};
        QApplication::setActiveWindow(&window);window.setFocus();
        const auto press=[&] {
            QKeyEvent override(QEvent::ShortcutOverride,Qt::Key_K,Qt::ControlModifier|Qt::ShiftModifier);
            QApplication::sendEvent(&window,&override);
            QKeyEvent key(QEvent::KeyPress,Qt::Key_K,Qt::ControlModifier|Qt::ShiftModifier);
            QApplication::sendEvent(&window,&key);settle();
        };
        press();check(aliasCalls==1,"Command Shift K opens customization on Edit despite a disjoint Fairlight binding");
        edit=false;press();check(aliasCalls==1,"Command Shift K keeps its native behavior outside the Edit context");
        edit=true;aliasOwners=QStringList{"EditTimeline.otherCommand"};press();
        check(aliasCalls==1,"Command Shift K refuses an overlapping Edit binding");
    }
    std::printf("native keyboard entry route failures=%d\n",failures);return failures?1:0;
}
