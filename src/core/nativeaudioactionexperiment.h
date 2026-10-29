// SPDX-License-Identifier: MIT
#pragma once
#include <QContextMenuEvent>
#include <QDateTime>
#include <QGraphicsSceneContextMenuEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QtMath>
#include <QMenu>
#include <QPointer>
#include <QTimer>
#include "audiochannelplanrequest.h"
#include "nativeaudioactionpermit.h"
#include "nativeaudiochannelchoice.h"
#include "nativetimelineselection.h"
namespace Unbound {
// Diagnostic only: one explicitly permitted native action on a manually prepared
// disposable clip. It does not create/show/prepare menus or supply a product flow.
class NativeAudioActionExperiment : public QObject {
public:
    explicit NativeAudioActionExperiment(QObject *owner,const QString &permitPath)
        :QObject(owner),permit(permitPath),planning(this,true),deadline(this) {
        deadline.setSingleShot(true);
        connect(&deadline,&QTimer::timeout,this,[this]{cancel("read-only-plan-timeout");});
    }
    ~NativeAudioActionExperiment()override{clearContext("owner-destroyed");}
    void observe(QObject *receiver,QEvent *event) {
        const auto type=event->type();
        if(type==QEvent::ApplicationDeactivate ||
           (menusClosed() && (type==QEvent::MouseButtonPress || type==QEvent::KeyPress))) {
            clearContext("intervening-user-input");
        }
        if(pending && (type==QEvent::KeyPress || type==QEvent::MouseButtonPress || type==QEvent::ApplicationDeactivate))
            cancel("intervening-input");
        if(type==QEvent::ContextMenu || type==QEvent::GraphicsSceneContextMenu) {
            clearContext("new-context-gesture");
            auto root=NativeTimelineSelection::focusedView();
            const auto selected=NativeTimelineSelection::inspect(root);
            if(!selected.reason.isEmpty() || !selected.view)return;
            QPoint global;
            if(type==QEvent::GraphicsSceneContextMenu) {
                if(!selected.scenes.contains(qobject_cast<QGraphicsScene *>(receiver)))return;
                global=static_cast<QGraphicsSceneContextMenuEvent *>(event)->screenPos();
            } else {
                auto widget=qobject_cast<QWidget *>(receiver);
                const bool rootOwned=widget && (root==widget || root->isAncestorOf(widget));
                const bool trackOwned=widget && (selected.view==widget || selected.view->isAncestorOf(widget));
                if(!rootOwned && !trackOwned)return;
                global=static_cast<QContextMenuEvent *>(event)->globalPos();
            }
            const auto point=selected.view->viewport()->mapFromGlobal(global);
            if(!selected.view->viewport()->rect().contains(point))return;
            auto hit=selected.scene->itemAt(selected.view->mapToScene(point),selected.view->viewportTransform());
            if(!hit || (hit!=selected.item && !selected.item->isAncestorOf(hit)))return;
            timeline=root;scene=selected.scene;item=selected.item;gestureAt=QDateTime::currentMSecsSinceEpoch();
            for(auto observed:selected.scenes)watches.append(connect(observed,&QGraphicsScene::selectionChanged,this,[this]{
                if(!sameSelection())clearContext("selection-changed");
            }));
        }
        if(type==QEvent::Show)if(auto shown=qobject_cast<QMenu *>(receiver)) {
            if(!sameSelection() || shown->property("unboundOwnMenu").toBool() ||
               QDateTime::currentMSecsSinceEpoch()-gestureAt>=30000)return;
            for(auto action:shown->actions()) {
                auto label=action->text();label.remove('&');
                if(label.trimmed()=="Audio Channel" && action->menu()) {
                    if(candidate && candidate!=action->menu()){clearContext();return;}
                    candidate=action->menu();rootMenu=shown;
                }
            }
            if(shown==candidate && shown->title().trimmed()=="Audio Channel" && rootMenu) {
                menu=shown;boundAt=QDateTime::currentMSecsSinceEpoch();
                for(auto observed:{menu.data(),rootMenu.data()}) {
                    if(contextMenus.contains(observed))continue;
                    contextMenus.append(observed);
                    watches.append(connect(observed,&QMenu::aboutToHide,this,[this,observed]{closedMenus.insert(observed);}));
                }
                logEvent("audio.action-experiment.context",{{"bound",true},{"source","actual-user-context-event"},{"native_operation_invoked",false}});
            }
        }
    }

    bool invoke(int direction,bool skipExcluded) {
        if(pending)return reject("operation-already-pending");
        if(direction!=-1 && direction!=1)return reject("invalid-direction");
        if(!menu || !sameSelection() || NativeTimelineSelection::focusedView()!=timeline ||
           QDateTime::currentMSecsSinceEpoch()-boundAt>120000)return reject("fresh-manually-bound-context-required");
        if(QApplication::activePopupWidget() || QApplication::activeModalWidget() || !menusClosed())
            return reject("native-menu-or-dialog-still-open");
        if(!permit.valid())return reject("explicit-one-shot-permit-required");
        if(permit.used())return reject("one-shot-permit-already-used");
        pending=true;deadline.start(2000);
        if(!planning.start(direction,skipExcluded,[this,direction,skipExcluded](QJsonObject plan){
            if(!pending)return;
            const auto entries=plan["items"].toArray();
            if(!permit.valid() || !plan["success"].toBool() || plan["phase"].toString()!="plan-only" ||
               plan["audio_selected"].toInt()!=1 || plan["video_selected"].toInt()!=0 || entries.size()!=1 ||
               plan["direction"].toInt()!=direction || plan["channel_scope"].toString()!=(skipExcluded?"skip-excluded":"all-source") ||
               !sameSelection() || NativeTimelineSelection::focusedView()!=timeline || !menu ||
               QApplication::activePopupWidget() || QApplication::activeModalWidget() || !menusClosed()) {cancel("context-or-permit-mismatch");return;}
            const auto entry=entries.at(0).toObject();
            if(!permit.matches(plan["project_id"].toString(),plan["timeline_id"].toString(),entry["item_id"].toString()) || entry["status"].toString()!="ready") {cancel("disposable-item-mismatch");return;}
            const int before=entry["before"].toInt(),after=entry["after"].toInt(),count=entry["source_channels"].toInt();
            if(before<1 || after<1 || count<2 || count>64 || before>count || after>count || before==after) {
                cancel("invalid-plan-channels");return;
            }
            const auto choice=NativeAudioChannelChoice::inspect(menu,direction,before,after,count);
            if(!choice.action){cancel(choice.reason);return;}
            if(!permit.claim(plan["project_id"].toString(),plan["timeline_id"].toString(),entry["item_id"].toString())) {
                cancel("one-shot-permit-not-claimable");return;
            }
            pending=false;deadline.stop();
            logEvent("audio.action-experiment.invoke",{{"before",choice.current},{"after",choice.target},
                {"item_id",entry["item_id"]},{"menu_visible",menu->isVisible()},{"method","QAction-trigger-only"}});
            // No menu navigation, active-action manipulation, API setter or
            // automatic Undo. The existing native action is invoked once.
            flushDiagnostics();
            choice.action->trigger();
            logEvent("audio.action-experiment.returned",{{"completion","native-action-triggered"},
                {"mapping_verified",false},{"audible_result_verified",false},{"undo_verified",false}});
            clearContext();
        })) {cancel("read-only-planner-unavailable");return false;}
        return true;
    }
private:
    NativeAudioActionPermit permit;AudioChannelPlanRequest planning;QTimer deadline;
    QPointer<QGraphicsView> timeline;QPointer<QGraphicsScene> scene;QPointer<QMenu> menu,candidate,rootMenu;
    QList<QPointer<QMenu>> contextMenus;QSet<QMenu *> closedMenus;
    QGraphicsItem *item=nullptr;QList<QMetaObject::Connection> watches;
    qint64 gestureAt=0,boundAt=0;bool pending=false;
    bool menusClosed()const {
        if(contextMenus.size()!=2)return false;
        for(auto observed:contextMenus)if(!observed || observed->isVisible() || !closedMenus.contains(observed))return false;
        return true;
    }
    bool sameSelection()const {
        if(!timeline || !scene || !item)return false;
        const auto current=NativeTimelineSelection::inspect(timeline);
        return current.reason.isEmpty() && current.scene==scene && current.item==item;
    }
    bool reject(const QString &reason){logEvent("audio.action-experiment.rejected",{{"reason",reason}});return false;}
    void cancel(const QString &reason){if(pending){pending=false;deadline.stop();planning.cancel();reject(reason);}}
    void clearContext(const QString &reason="context-discarded"){
        if(menu)logEvent("audio.action-experiment.context-cleared",{{"reason",reason}});
        cancel(reason);for(auto watch:watches)disconnect(watch);watches.clear();
        timeline=nullptr;scene=nullptr;menu=nullptr;candidate=nullptr;rootMenu=nullptr;contextMenus.clear();closedMenus.clear();item=nullptr;gestureAt=0;boundAt=0;}
};
}
