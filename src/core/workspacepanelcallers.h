// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QTimer>
#include <functional>
#include <QJsonObject>
namespace Unbound {
// One current-page visibility model for native and custom workspace entry points.
// Showing/hiding never changes placement or switches Resolve pages.
class WorkspacePanelCallers : public QObject {
public:
    struct Panel {
        QString id,label,page;
        std::function<bool()> available,visible;
        std::function<void()> show;
        std::function<void(bool)> setVisible={};
    };
    explicit WorkspacePanelCallers(QObject *owner=nullptr) : QObject(owner) {}
    std::function<void(QString,QJsonObject)> report;
    std::function<QString()> currentPage=[] { return QString(); };
    void add(Panel panel) { panels.append(std::move(panel)); }
    static QString label(const Panel &panel) {
        if(panel.id=="unboundWorkspaceToolbar")return QStringLiteral("Workspace Toolbar");
        if(panel.id=="page-navigation")return QStringLiteral("Page Navigation");
        return panel.label;
    }
    void populateFlat(QMenu *menu) {
        const auto page=currentPage();
        for(int i=0;i<panels.size();++i) {
            const auto &panel=panels[i];
            if(panel.page!="global" && panel.page!=page)continue;
            auto action=menu->addAction(label(panel));action->setObjectName("openGatePanelToggle_"+panel.id);
            bind(action,i);update(action,panel,page);
        }
    }
    QMenu *wire(QMenu *root) {
        const auto page=currentPage();
        auto menu=root->findChild<QMenu *>("openGateShowPanel",Qt::FindDirectChildrenOnly);
        if(!menu) {
            menu=new QMenu("Panels",root);menu->setObjectName("openGateShowPanel");menu->setProperty("unboundOwnMenu",true);root->addMenu(menu);
        }
        for(int i=0;i<panels.size();++i) {
            const auto &panel=panels[i];
            auto action=menu->findChild<QAction *>("openGateShowPanel_"+panel.id,Qt::FindDirectChildrenOnly);
            if(!action){action=menu->addAction(label(panel));action->setObjectName("openGateShowPanel_"+panel.id);bind(action,i);}
            update(action,panel,page);
        }
        if(!watched.contains(menu)) {
            watched.append(menu);connect(menu,&QMenu::aboutToShow,this,[this,root]{wire(root);});
        }
        return menu;
    }

private:
    void update(QAction *action,const Panel &panel,const QString &page) {
        const bool pageAvailable=panel.page=="global" || panel.page==page;
        action->setText(label(panel));action->setCheckable(true);action->setVisible(pageAvailable);
        action->setEnabled(pageAvailable && panel.available && panel.available());
        action->setChecked(panel.visible && panel.visible());
        action->setToolTip("Show or hide "+label(panel)+" in its remembered location");
    }
    void record(const QString &event,const QJsonObject &details) {if(report)report(event,details);}
    void bind(QAction *action,int index) {
        connect(action,&QAction::triggered,this,[this,index](bool value) {
            const auto requestedPage=currentPage();
            QTimer::singleShot(0,this,[this,index,value,requestedPage] {
                const auto &panel=panels[index];
                const bool ready=currentPage()==requestedPage && (panel.page=="global" || currentPage()==panel.page) && panel.available && panel.available();
                record("workspace.panel.visibility.request",{{"panel",panel.id},{"page",currentPage()},{"requested",value},{"accepted",ready},{"origin","panels-menu"}});
                if(!ready){record("workspace.panel.visibility.failed",{{"panel",panel.id},{"reason","page-or-panel-unavailable"}});return;}
                if(panel.setVisible)panel.setVisible(value);else if(value && panel.show)panel.show();
                record("workspace.panel.visibility.dispatched",{{"panel",panel.id},{"requested",value},{"observed_visible",panel.visible && panel.visible()}});
            });
        });
    }
    QList<Panel> panels; QList<QPointer<QMenu>> watched;
};
}
