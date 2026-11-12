// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/speededitorcustomization.h"
namespace OpenGateTest {
template<class Check> void speedEditorBehaviorPreviewChecks(QWidget *page,Check check) {
    auto diagram=dynamic_cast<Unbound::SpeedEditorDiagram *>(page->findChild<QWidget *>("openGateSpeedEditorDiagram"));
    auto behavior=page->findChild<QComboBox *>("openGateSpeedEditorBehaviorPreview");
    auto gesture=page->findChild<QComboBox *>("openGateSpeedEditorGesturePreview");
    auto native=page->findChild<QComboBox *>("openGateSpeedEditorAssignmentPreview");
    auto help=page->findChild<QLabel *>("openGateSpeedEditorModeHelp");
    check(diagram && behavior && gesture && native && help,"preview exposes behavior, gesture and Native assignment");
    if(!diagram||!behavior||!gesture||!native||!help)return;
    diagram->clearSelection();
    check(!behavior->isEnabled() && help->isHidden() && gesture->isHidden(),"empty selection has no irrelevant mode explanation");
    diagram->button("smart_insrt")->click();
    check(behavior->currentText()=="Button actions" && behavior->isEnabled() && help->isHidden(),"ordinary button previews actions without mode help");
    QStringList options;for(int i=0;i<gesture->count();++i)options.append(gesture->itemText(i));
    check(options==QStringList{"Single press","Double press","Hold","Hold + dial left","Hold + dial right"},"action gestures expose the complete agreed dropdown");
    for(int i=0;i<gesture->count();++i){gesture->setCurrentIndex(i);check(native->currentData()=="native","each gesture initially preserves Native independently");}
    behavior->setCurrentIndex(1);
    check(!help->isHidden() && gesture->count()==2 && gesture->itemText(0)=="Dial left" &&
          gesture->itemText(1)=="Dial right" && native->isHidden(),"Jogwheel mode presents directional control without independent action assignments");
    behavior->setCurrentIndex(0);
    check(help->isHidden() && gesture->count()==5 && !native->isHidden(),"switching back restores action preview and removes mode explanation");
    diagram->button("jog")->click();
    check(behavior->currentText()=="Jogwheel mode" && !help->isHidden(),"native Jog button previews its mode role");
    diagram->clearSelection();
    check(help->isHidden() && native->isHidden(),"clearing selection removes assignment and mode details");
    for(auto button:page->findChildren<QPushButton *>())if(button->text()=="Save"||button->text()=="Reset Key to Native")
        check(!button->isEnabled(),"preview cannot save or mutate the Native profile");
}
}
