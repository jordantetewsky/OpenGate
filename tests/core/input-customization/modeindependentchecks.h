// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelcontrols.h"

namespace OpenGateTest {
inline QDialog *inputEditorFixture(QWidget *parent,QString &) {
    auto dialog=new QDialog(parent);
    dialog->setObjectName("UiActionsEditorDialog");dialog->resize(800,580);
    auto layout=new QVBoxLayout(dialog);
    auto header=new QWidget(dialog);auto row=new QHBoxLayout(header);
    auto options=new QToolButton(header);options->setObjectName("btnOptions");
    row->addWidget(new QLabel(Unbound::InputCustomizationPanel::label(),header));row->addWidget(options);
    layout->addWidget(header);layout->addStretch();
    return dialog;
}
inline Unbound::ManagedPanelSession *inputSession(PanelControls &controls) {
    for(auto object:controls.children())if(auto session=dynamic_cast<Unbound::ManagedPanelSession *>(object))
        if(session->window() && session->window()->objectName()=="unboundFloating_"+Unbound::InputCustomizationPanel::id())return session;
    return nullptr;
}
inline QJsonObject savedInputWorkspace() {
    return Unbound::WorkspaceSettings().pageView("edit");
}
// Exercise the real coordinator, caller menu, Native-return callback and preset
// transaction using the caller-flow harness's existing native surface tree.
template<class Check,class Settle>
void inputModeIndependentChecks(PanelControls &controls,QMenu *ui,
                                Unbound::PanelCallerMenu *callers,QWidget *main,QWidget *timeline,
                                Check check,Settle settle) {
    auto native=ui?ui->findChild<QAction *>("unboundRestoreNativeWorkspace"):nullptr;
    auto open=[&] {
        QMenu menu;callers->populateFlat(&menu);
        for(auto action:menu.actions())if(action->text()==Unbound::InputCustomizationPanel::label()) {
            action->trigger();settle(350);return;
        }
        check(false,"input customization caller is registered");
    };
    check(native && Unbound::PageWorkspaceState::mode(savedInputWorkspace())=="native",
          "input checks begin through real Native-return action");
    if(!native)return;
    const auto mainFrame=main->geometry();
    open();auto session=inputSession(controls);
    check(session && session->active(),"Native-mode caller opens customization session");
    if(!session)return;
    auto host=session->window();
    check(Unbound::PageWorkspaceState::mode(savedInputWorkspace())=="native" && main->geometry()==mainFrame,
          "opening customization keeps Native mode and main window size");
    check(session->setInterfaceScale(1.4),"content scale is available in Native mode");
    host->setGeometry(160,130,900,650);settle(500);
    const auto frame=host->geometry();
    auto saved=savedInputWorkspace()["panels"].toObject()[Unbound::InputCustomizationPanel::id()].toObject();
    check(Unbound::WorkspaceSettings::rectangle(saved)==frame && saved["interface_scale"].toDouble()==1.4,
          "Native-mode geometry and scale use existing workspace panel record");
    native->trigger();settle(400);
    check(session->active() && session->window()==host && host->geometry()==frame && session->interfaceScale()==1.4,
          "Native return retains customization host geometry and scale");
    auto content=host->findChild<QTabWidget *>(Unbound::InputCustomizationPanel::id());
    if(content)qobject_cast<QDialog *>(content->widget(0))->reject();
    settle(250);
    check(session->active() && session->hiddenByUser(),"native editor Close hides retained Native-mode session");
    open();
    check(session->window()==host && !session->hiddenByUser() && host->geometry()==frame,
          "Native-mode recall preserves the same window and geometry");
    // Moving through the production title-drag callback must not select a
    // workspace. Finish the synthetic gesture without sending an actual drop.
    auto page=main->findChild<QWidget *>("WStackPage_Conform");
    auto pageStack=page?qobject_cast<QStackedWidget *>(page->parentWidget()):nullptr;
    if(pageStack && pageStack->count()>1) {
        pageStack->setCurrentIndex(1);settle(100);pageStack->setCurrentWidget(page);settle(400);
    }
    bool dragCalled=false;
    for(auto button:host->findChildren<QToolButton *>())if(auto door=dynamic_cast<Unbound::NativeUndockButton *>(button))
        if(door->drag){door->drag(host->mapToGlobal(QPoint(30,15)));dragCalled=true;break;}
    settle(200);
    check(dragCalled && Unbound::PageWorkspaceState::mode(savedInputWorkspace())=="native",
          "customization title drag does not enter managed mode");
    host->setGeometry(frame);settle(350);
    if(session->attachRequested)session->attachRequested(timeline,Qt::RightEdge);
    settle(500);
    check(session->grouped() && Unbound::PageWorkspaceState::mode(savedInputWorkspace())=="managed",
          "explicit customization dock enters managed mode");
    native->trigger();settle(450);
    check(session->active() && !session->grouped() && session->window()==host &&
              session->floatingGeometry()==frame && session->interfaceScale()==1.4,
          "Native return extracts grouped customization and restores its floating frame");
    Unbound::WorkspaceRestoration *restore=nullptr;
    for(auto child:controls.children())if(auto candidate=dynamic_cast<Unbound::WorkspaceRestoration *>(child))restore=candidate;
    auto entry=Unbound::resolveLayoutEntry("edit");auto value=entry["workspace"].toObject();
    auto panels=value["panels"].toObject();
    panels[Unbound::InputCustomizationPanel::id()]=QJsonObject{{"floating",false},{"interface_scale",1.0},{"x",5},{"y",5},{"width",300},{"height",200}};
    value["panels"]=panels;entry["workspace"]=value;
    if(restore)restore->preset(entry,true);
    settle(1600);
    check(restore && !restore->active() && session->active() && session->window()==host &&
              host->geometry()==frame && session->interfaceScale()==1.4,
          "workspace preset transaction preserves independent window size and scale");
    auto incomingNative=value["native_arrangement"].toObject();
    incomingNative["tree"]=Unbound::insertGroupTree(incomingNative["tree"].toObject(),
        {{"panel",Unbound::InputCustomizationPanel::id()}},"frameConformTimelines",Qt::Horizontal,false);
    auto nativePanels=incomingNative["panels"].toObject();
    nativePanels[Unbound::InputCustomizationPanel::id()]=QJsonObject{{"hidden",false}};
    incomingNative["panels"]=nativePanels;value["native_arrangement"]=incomingNative;entry["workspace"]=value;
    if(restore)restore->preset(entry,true);settle(1500);
    check(restore && !restore->active() && session->window()==host && !session->grouped() &&
              host->geometry()==frame && session->interfaceScale()==1.4 &&
              !savedInputWorkspace()["native_arrangement"].toObject()["panels"].toObject().contains(Unbound::InputCustomizationPanel::id()),
          "preset native membership cannot reclaim independent input window");
    native->trigger();settle(400);
    value=savedInputWorkspace();value["layout_mode"]="managed";
    const auto tree=Unbound::splitGroupTree({{"panel",Unbound::InputCustomizationPanel::id()}},
                                          {{"panel","frameConformTimelines"}},Qt::Horizontal);
    auto group=Unbound::WorkspaceSettings::rectangle(QRect(180,160,1000,600));group["tree"]=tree;
    value["groups"]=QJsonArray{group};entry["workspace"]=value;
    if(restore)restore->preset(entry,true);settle(1300);
    check(restore && !restore->active() && session->window()==host && !session->grouped() &&
              host->geometry()==frame && session->interfaceScale()==1.4 && savedInputWorkspace()["groups"].toArray().isEmpty(),
          "preset floating group cannot reclaim independent input window");
    native->trigger();settle(400);
    if(content)qobject_cast<QDialog *>(content->widget(0))->reject();
    settle(400);
}

template<class Check,class Settle>
void inputModeIndependentRestartChecks(Check check,Settle settle) {
    const auto before=savedInputWorkspace()["panels"].toObject()[Unbound::InputCustomizationPanel::id()].toObject();
    PanelControls controls(inputEditorFixture);settle(700);
    auto session=inputSession(controls);
    check(session && session->active() && session->hiddenByUser(),
          "coordinator restart restores retained hidden customization in Native mode");
    if(!session)return;
    auto tabs=session->window()->findChild<QTabWidget *>(Unbound::InputCustomizationPanel::id());
    check(tabs && tabs->currentIndex()==1 && tabs->currentWidget()->objectName()==before["selected_tab"].toString(),
          "coordinator restart restores selected Speed Editor tab from session record");
    check(Unbound::PageWorkspaceState::mode(savedInputWorkspace())=="native" &&
              session->floatingGeometry()==Unbound::WorkspaceSettings::rectangle(before) &&
              session->interfaceScale()==before["interface_scale"].toDouble(),
          "coordinator restart retains Native mode geometry and content scale");
}
}
