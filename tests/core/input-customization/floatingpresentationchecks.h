// SPDX-License-Identifier: MIT
#pragma once
#include "modeindependentchecks.h"

namespace OpenGateTest {
template<class Check,class Settle>
void inputFloatingPresentationChecks(PanelControls &controls,Unbound::ManagedPanelSession *session,
                                     QTabWidget *content,Check check,Settle settle) {
    if(!session || !content){check(false,"floating input fixture has its content");return;}
    auto host=session->window();const auto original=host->geometry();
    auto screen=host->screen();
    const auto margins=host->windowHandle()?host->windowHandle()->frameMargins():QMargins();
    const auto available=screen?screen->availableGeometry().marginsRemoved(margins):QRect();
    const auto desired=content->property("openGateFloatingMinimumSize").toSize();
    check(desired.isValid() && host->minimumSize()==desired.boundedTo(available.size()),
          "input floating host has a usable baseline minimum bounded by its screen");
    host->resize(100,80);settle(250);
    check(host->width()>=host->minimumWidth() && host->height()>=host->minimumHeight(),
          "input host cannot collapse below its usable minimum");
    // A screen smaller than the native baseline must remain usable through
    // overflow. Exercise that boundary without changing any real display mode.
    content->setProperty("openGateFloatingMinimumSize",available.size()*2);
    session->setFloatingGeometry(original);settle(250);
    check(host->minimumSize()==available.size(),"oversized native baseline is capped to usable screen size");
    content->setProperty("openGateFloatingMinimumSize",desired);
    session->setFloatingGeometry(original);settle(250);
    auto expand=controls.findChild<QPushButton *>("expand_"+Unbound::InputCustomizationPanel::id());
    check(expand,"input panel uses existing coordinator expansion entry");
    if(expand)expand->click();settle(450);
    check(host->property("unboundExpanded").toBool() && host->geometry()==available,
          "floating input expansion fills current usable screen instead of main workspace viewport");
    content->setCurrentIndex(1);settle(400);
    const auto expandedSaved=savedInputWorkspace()["panels"].toObject()[Unbound::InputCustomizationPanel::id()].toObject();
    check(Unbound::WorkspaceSettings::rectangle(expandedSaved)==original,
          "expanded input persistence retains original floating rectangle");
    check(expandedSaved["selected_tab"]==content->widget(1)->objectName(),
          "tab selection persists during expansion without replacing its original rectangle");
    if(expand)expand->click();settle(400);
    check(!host->property("unboundExpanded").toBool() && host->geometry()==original,
          "second expansion command restores exact floating geometry");
    content->setCurrentIndex(1);settle(400);
    check(savedInputWorkspace()["panels"].toObject()[Unbound::InputCustomizationPanel::id()].toObject()["selected_tab"]==
              content->widget(1)->objectName(),"device tab selection uses existing session persistence");
}
}
