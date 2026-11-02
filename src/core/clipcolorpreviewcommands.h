// SPDX-License-Identifier: MIT
#pragma once
#include <QScopedValueRollback>
#include "panelcommands.h"
#include "nativeclipcolormenus.h"
#include "menutrackingcommand.h"
#include "timelineclipcolorpreview.h"
#include "mediapoolclipcolorpreview.h"
namespace Unbound {
// Wire the existing command registry and native menus to surface adapters.
// This coordinator implements no painting, scene mutation or delegate lifecycle.
class ClipColorPreviewCommands : public QObject {
public:
    ClipColorPreviewCommands(QObject *owner,PanelCommands *commands):QObject(owner),commands(commands),pool(this) {
        QPointer<ClipColorPreviewCommands> alive=this;
        for(const auto &surface:surfaces())for(const QString &operation:{QString("red"),QString("clear")}) {
            const auto id="clip-color."+surface.key+"."+operation;
            commands->add({id,surface.label+" Clip Color / "+(operation=="red"?"Preview Red (Temporary)":"Clear Temporary Preview"),
                [alive,operation]{return alive && alive->available(alive->focus(),operation);},
                [alive,operation]{if(alive)alive->apply(alive->focus(),operation);},surface.panel});
        }
        new NativeClipColorMenus(this,[this](QMenu *menu,QWidget *origin){prepare(menu,origin);});
    }
    ~ClipColorPreviewCommands() override {
        for(const auto &entry:menus)for(auto action:{entry.separator,entry.red,entry.clear})if(action)delete action;
        for(auto adapter:timelines)if(adapter)delete adapter;
    }
private:
    struct Surface {QString key,label,panel;};
    struct Menu {QPointer<QWidget> origin;QPointer<QAction> separator,red,clear;};
    QPointer<PanelCommands> commands;MediaPoolClipColorPreview pool;QPointer<QWidget> explicitFocus;
    QHash<QGraphicsView *,QPointer<TimelineClipColorPreview>> timelines;QHash<QMenu *,Menu> menus;
    static QList<Surface> surfaces(){
        QList<Surface> result{{"timeline","Timeline","frameConformTimelines"}};
        if(qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_MEDIA_POOL_COLORS")==1)
            result.append({"media-pool","Media Pool","frameEditPageMediaPool"});
        return result;
    }
    static PanelCommandContext::Target target(QWidget *widget) {
        return PanelCommandContext::capture(widget,{"frameConformTimelines","frameEditPageMediaPool"});
    }
    QWidget *focus() const {return explicitFocus?explicitFocus.data():QApplication::focusWidget();}
    TimelineClipColorPreview *timeline(QWidget *widget) {
        const auto panel=target(widget);if(panel.panel!="frameConformTimelines")return nullptr;
        QGraphicsView *view=nullptr;
        for(auto p=widget;p;p=panelParent(p))if(p->objectName()=="m_TiTimelineView")view=qobject_cast<QGraphicsView *>(p);
        if(!view)for(auto candidate:QApplication::allWidgets())if(candidate->objectName()=="m_TiTimelineView" &&
            candidate->isVisible() && panelContains(panel.surface,candidate)) {
            auto found=qobject_cast<QGraphicsView *>(candidate);if(!found)continue;if(view)return nullptr;view=found;
        }
        if(!view)return nullptr;
        if(!timelines.value(view)) {
            timelines[view]=new TimelineClipColorPreview(view,view);
            connect(view,&QObject::destroyed,this,[this,view]{timelines.remove(view);});
        }
        return timelines.value(view);
    }
    bool available(QWidget *widget,const QString &operation) {
        if(!widget || textEditor(widget))return false;
        const auto panel=target(widget);if(!panel.surface)return false;
        if(panel.panel=="frameConformTimelines") {auto adapter=timeline(widget);return adapter && (operation=="clear" || adapter->hasSelection());}
        return qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_MEDIA_POOL_COLORS")==1 &&
            panel.panel=="frameEditPageMediaPool" && (operation=="clear" || pool.hasSelection(panel.surface));
    }
    void apply(QWidget *widget,const QString &operation) {
        if(!available(widget,operation))return;
        const auto panel=target(widget);
        if(panel.panel=="frameConformTimelines") {auto adapter=timeline(widget);if(operation=="clear")adapter->clear();else adapter->applySelected(Qt::red);}
        else if(operation=="clear")pool.clear(panel.surface);else pool.applySelected(panel.surface,Qt::red);
    }
    QString id(QWidget *origin,const QString &operation) const {
        const auto panel=target(origin);for(const auto &surface:surfaces())if(surface.panel==panel.panel)return "clip-color."+surface.key+"."+operation;
        return {};
    }
    void queue(QMenu *menu,const QString &operation) {
        const auto origin=menus.value(menu).origin;const auto command=id(origin,operation);
        MenuTrackingCommand::queue(this,[this,origin,command]{
            if(!origin || command.isEmpty() || !commands)return;
            QScopedValueRollback<QPointer<QWidget>> guard(explicitFocus,origin);
            commands->invoke(command,"opengate-command-menu");
        });
    }
    void prepare(QMenu *menu,QWidget *origin) {
        const bool supported=!id(origin,"red").isEmpty();
        if(!supported){if(menus.contains(menu))for(auto action:{menus[menu].separator,menus[menu].red,menus[menu].clear})if(action)action->setVisible(false);return;}
        if(!menus.contains(menu)) {
            connect(menu,&QObject::destroyed,this,[this,menu]{menus.remove(menu);});menus.insert(menu,{});
        }
        auto &entry=menus[menu];
        if(!entry.red || !entry.clear) {
            for(auto action:{entry.separator,entry.red,entry.clear})if(action)delete action;
            entry.separator=menu->addSeparator();
            entry.red=menu->addAction("Preview Red (Temporary)");entry.red->setObjectName("openGateRedClipPreview");
            entry.clear=menu->addAction("Clear Temporary Clip Color Preview");
            connect(entry.red,&QAction::triggered,this,[this,menu]{queue(menu,"red");});
            connect(entry.clear,&QAction::triggered,this,[this,menu]{queue(menu,"clear");});
        }
        entry.origin=origin;
        for(auto action:{entry.separator,entry.red,entry.clear})if(action)action->setVisible(true);
        entry.red->setProperty("openGateCommandId",id(origin,"red"));entry.clear->setProperty("openGateCommandId",id(origin,"clear"));
        entry.red->setEnabled(available(origin,"red"));entry.clear->setEnabled(available(origin,"clear"));
        logEvent("clip-color.menu.prepared",{{"panel",target(origin).panel},{"red_enabled",entry.red->isEnabled()},
            {"native_name",menu->objectName()},{"temporary",true}});
    }
};
}
