// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QElapsedTimer>
#include "panelcommands.h"
namespace Unbound {
// Independent presentation of command IDs. Resolve-owned workflow actions stay
// in place for native keyboard customization and script execution.
class PanelCommandMenu : public QObject {
public:
    PanelCommandMenu(QObject *owner,PanelCommands *commands):QObject(owner),commands(commands) {}
    ~PanelCommandMenu() override {for(auto menu:menus)if(menu)delete menu;}
    QMenu *menu(QMenu *root) {
        if(!root || !commands)return nullptr;
        for(auto menu:menus)if(menu && menu->parentWidget()==root)return menu;
        auto menu=new QMenu("Commands",root);menu->setObjectName("openGateCommands");menu->setProperty("unboundOwnMenu",true);menus.append(menu);
        auto expand=new QMenu("Maximize / Restore",menu);expand->setProperty("unboundOwnMenu",true);
        auto visibility=new QMenu("Show / Hide",menu);visibility->setProperty("unboundOwnMenu",true);
        for(const auto &command:commands->entries()) {
            QMenu *target=nullptr;QString label;
            if(command.id=="edit.inspector.stabilize"){target=menu;label="Stabilize Selected Clips";}
            else if(command.id=="edit.timeline.fixed-playhead"){target=menu;label="Toggle Fixed Playhead";}
            else if(command.id.startsWith("panel.expand.") && command.id!="panel.expand.hovered"){
                target=expand;label=command.label;label.remove("Maximize / Restore ");
            } else if(command.id.startsWith("panel.toggle.")){
                target=visibility;label=command.label;label.remove("Show / Hide ");
            }
            // Other registered operations use their catalog labels; adding a
            // command must not require a second implementation in the menu.
            if(!target && !command.id.startsWith("panel.")){target=menu;label=command.label;}
            if(!target)continue;
            auto action=target->addAction(label);action->setProperty("openGateCommandId",command.id);
            connect(action,&QAction::triggered,this,[this,id=command.id]{queue(id);});
            auto available=command.available;
            auto refresh=[action,available,registry=QPointer<PanelCommands>(commands)] {action->setEnabled(registry && available && available());};
            connect(target,&QMenu::aboutToShow,action,refresh);refresh();
        }
        if(!expand->actions().isEmpty())menu->addMenu(expand);
        if(!visibility->actions().isEmpty())menu->addMenu(visibility);
        return menu;
    }
private:
    QPointer<PanelCommands> commands;QList<QPointer<QMenu>> menus;
    void queue(const QString &id){
        auto timer=new QTimer(this);timer->setSingleShot(true);QElapsedTimer elapsed;elapsed.start();
        connect(timer,&QTimer::timeout,this,[this,timer,id,elapsed]{
            if(QApplication::activePopupWidget() && elapsed.elapsed()<15000){timer->start(25);return;}
            if(commands && !QApplication::activePopupWidget())commands->invoke(id,"opengate-command-menu");
            else logEvent("command.menu.cancelled",{{"command",id},{"reason","menu-tracking-timeout-or-owner-unavailable"}});
            timer->deleteLater();
        });timer->start(0);
    }
};
}
