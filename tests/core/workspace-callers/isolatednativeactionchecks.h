// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/isolatednativeaction.h"
#include <QHideEvent>
#include <QKeyEvent>
namespace OpenGateTest {
template<class Check> void isolatedNativeActionChecks(QObject *owner,Check check) {
    QMenu root,palette(&root),project;
    root.addMenu(&palette);
    auto custom=Unbound::IsolatedNativeAction::create("Custom",&palette,[]{});
    palette.addAction(custom);
    int projectCloses=0,paletteCloses=0,nativeCalls=0;
    QObject::connect(&project,&QMenu::aboutToHide,owner,[&]{++projectCloses;});
    QObject::connect(&palette,&QMenu::aboutToHide,owner,[&]{++paletteCloses;});
    auto native=project.addAction("Native project action");
    QObject::connect(&project,&QMenu::triggered,owner,[&](QAction *){++nativeCalls;});
    custom->hover();
    check(palette.signalsBlocked()&&root.signalsBlocked(),
          "custom action isolates only its actual native propagation chain");
    check(!project.signalsBlocked(),"custom hover never blocks an unrelated native menu");
    QMetaObject::invokeMethod(&project,"aboutToHide",Qt::DirectConnection);
    check(projectCloses==1,"unrelated native lifecycle notifications survive custom hover");
    QHideEvent hiding;QApplication::sendEvent(&palette,&hiding);
    check(paletteCloses==1&&!palette.signalsBlocked()&&!root.signalsBlocked(),
          "native menu hide restores leases before its lifecycle notification");
    custom->trigger();native->trigger();
    check(nativeCalls==1&&!project.signalsBlocked(),
          "a following unrelated native command remains deliverable without a timer turn");
    QKeyEvent input(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    // Use a non-mouse receiver: the global lease filter handles the type first.
    QCoreApplication::sendEvent(owner,&input);
}
}
