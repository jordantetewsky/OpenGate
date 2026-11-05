// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/nativeprojectselectorinspection.h"
namespace OpenGateTest {
template<class Check> void projectSelectorInspectionChecks(QPushButton *button,Check check) {
    Unbound::NativeProjectSelectorInspection inspection(nullptr);
    inspection.observe(button);inspection.observe(button);
    auto state=inspection.capture(button);
    check(state["push_button_interface"].toBool()&&!state["menu_attached"].toBool(),
          "selector inspection distinguishes a native null menu from a popup failure");
    QMenu native(button);native.addAction("Other project");
    auto previous=button->menu();button->setMenu(&native);
    const auto parent=native.parent();const auto flags=native.windowFlags();
    state=inspection.capture(button);
    check(state["menu_attached"].toBool()&&state["menu_actions"].toInt()==1&&
              button->menu()==&native&&native.parent()==parent&&native.windowFlags()==flags,
          "selector inspection retains the native menu, actions, owner and flags");
    native.blockSignals(true);state=inspection.capture(button);
    check(state["menu_signals_blocked"].toBool()&&native.signalsBlocked(),
          "selector inspection reports signal state without repairing or changing it");
    native.blockSignals(false);button->setMenu(nullptr);
    int calls=0;auto connection=QObject::connect(button,&QPushButton::clicked,[&]{++calls;});
    const auto point=button->rect().center();
    QMouseEvent press(QEvent::MouseButtonPress,point,button->mapToGlobal(point),
                      Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,point,button->mapToGlobal(point),
                        Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(button,&press);QApplication::sendEvent(button,&release);
    check(calls==1&&button->menu()==nullptr&&!button->signalsBlocked(),
          "selector diagnostic does not consume, duplicate or replace native clicks");
    QObject::disconnect(connection);button->setMenu(previous);
}
}
