// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include "nativeaudiochannelchoice.h"
#include "nativetimelineselection.h"
#include "audiochannelplanrequest.h"
#include "diagnostics.h"
namespace Unbound {
// One native selected-item menu transaction. No API mapping setter, cached menu,
// focus movement, track selection, or cursor movement is used.
class NativeAudioChannelTransaction : public QObject {
public:
    explicit NativeAudioChannelTransaction(QObject *owner):QObject(owner),deadline(this),planning(this) {
        deadline.setSingleShot(true);
        connect(&deadline,&QTimer::timeout,this,[this]{finish("native-menu-timeout");});
    }
    ~NativeAudioChannelTransaction() override { if(qApp){finish("native-channel-owner-destroyed");qApp->removeEventFilter(this);} }
    bool start(int step,bool skipExcluded=false) {
        auto reject=[](const QString &reason) {
            logEvent("command.audio-channel.native-rejected",{{"reason",reason}});return false;
        };
        if(active)return reject("native-channel-request-already-active");
        if(step!=-1 && step!=1)return reject("invalid-direction");
        if(QApplication::activeModalWidget() || QApplication::activePopupWidget())return reject("native-dialog-or-popup-active");
        auto view=NativeTimelineSelection::focusedView();
        if(!view || !view->isVisible() || !view->scene())return reject("focused-native-timeline-scene-unavailable");
        const auto selected=NativeTimelineSelection::inspect(view);
        logEvent("command.audio-channel.native-selection",{{"inspected_scenes",selected.scenes.size()},
            {"view_class",view->metaObject()->className()},{"scene_class",view->scene()->metaObject()->className()},
            {"selection_scene_class",selected.scene?selected.scene->metaObject()->className():""}});
        if(!selected.reason.isEmpty())return reject(selected.reason);
        timeline=view;scene=selected.scene;item=selected.item;direction=step;active=true;issued=false;
        for(auto observed:selected.scenes)selectionWatches.append(connect(observed,&QGraphicsScene::selectionChanged,this,[this] {
            if(active && !sameSelection())finish("native-selection-changed");
        }));
        root=nullptr;channelMenu=nullptr;
        qApp->installEventFilter(this);deadline.start(2000);
        pendingView=selected.view;
        if(!planning.start(step,skipExcluded,[this](QJsonObject plan){
            if(!active)return;
            const auto entries=plan["items"].toArray();
            if(!plan["success"].toBool() || plan["phase"].toString()!="plan-only" ||
               plan["audio_selected"].toInt()!=1 || plan["video_selected"].toInt()!=0 || entries.size()!=1 ||
               !sameSelection()) {finish("unsupported-or-changed-audio-plan");return;}
            const auto entry=entries.at(0).toObject();
            if(entry["status"].toString()!="ready") {finish(entry["reason"].toString());return;}
            expectedCurrent=entry["before"].toInt();plannedTarget=entry["after"].toInt();sourceCount=entry["source_channels"].toInt();
            if(expectedCurrent<1 || plannedTarget<1 || sourceCount<2 || expectedCurrent>sourceCount || plannedTarget>sourceCount) {
                finish("invalid-audio-plan-channels");return;
            }
            openMenu();
        })) {finish("native-audio-planner-unavailable");return false;}

        return true;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(!active)return false;
        if(event->type()==QEvent::ApplicationDeactivate) {finish("application-deactivated");return false;}
        if(!sending && (event->type()==QEvent::MouseButtonPress || event->type()==QEvent::KeyPress)) {
            finish("input-changed-during-native-channel-request");return false;
        }
        if(event->type()!=QEvent::Show)return false;
        auto menu=qobject_cast<QMenu *>(object);
        if(!menu || menu->property("unboundOwnMenu").toBool())return false;
        if(channelMenu==menu) {
            QPointer<QMenu> safe=menu;
            QTimer::singleShot(0,this,[this,safe] {if(active && safe)choose(safe);});
        } else if(!root) {
            root=menu;
            QAction *entry=nullptr;
            for(auto action:menu->actions()) {
                auto title=action->text();title.remove('&');
                if(title.trimmed()=="Audio Channel" && action->menu()) {
                    if(entry){finish("ambiguous-native-audio-menu");return false;}
                    entry=action;
                }
            }
            if(!entry) {
                QTimer::singleShot(0,this,[this]{finish("selected-item-has-no-native-audio-menu");});
                return false;
            }
            root=menu;channelMenu=entry->menu();QPointer<QAction> safeEntry=entry;
            QTimer::singleShot(0,this,[this,safeEntry] {
                if(!active || !root || !safeEntry)return;
                if(!sameSelection()){finish("native-selection-changed");return;}
                root->setActiveAction(safeEntry);
                sendKey(root,Qt::Key_Right);
            });
        }
        return false;
    }
private:
    void openMenu() {
        if(!sameSelection() || !pendingView){finish("native-selection-changed");return;}
        QGraphicsSceneContextMenuEvent event(QEvent::GraphicsSceneContextMenu);
        event.setReason(QGraphicsSceneContextMenuEvent::Keyboard);
        event.setPos(item->boundingRect().center());
        event.setScenePos(item->mapToScene(event.pos()));
        event.setScreenPos(pendingView->viewport()->mapToGlobal(pendingView->mapFromScene(event.scenePos())));
        scene->sendEvent(item,&event);
    }
    bool sameSelection() const {
        if(!timeline || !scene)return false;
        const auto current=NativeTimelineSelection::inspect(timeline);
        return current.reason.isEmpty() && current.scene==scene && current.item==item;
    }
    void sendKey(QWidget *target,int key) {
        sending=true;
        QKeyEvent press(QEvent::KeyPress,key,Qt::NoModifier);
        QApplication::sendEvent(target,&press);
        sending=false;
    }
    void choose(QMenu *menu) {
        if(issued)return;
        if(!sameSelection()){finish("native-selection-changed");return;}
        const auto choice=NativeAudioChannelChoice::inspect(menu,direction,expectedCurrent,plannedTarget,sourceCount);
        if(!choice.action){finish(choice.reason);return;}
        issued=true;menu->setActiveAction(choice.action);
        // Use native QMenu activation so exec()-return handlers as well as
        // QAction signal handlers receive the selection exactly as a user would.
        sendKey(menu,Qt::Key_Return);
        logEvent("command.audio-channel.native-choice",{{"before",choice.current},{"after",choice.target},
            {"completion","native-menu-key-delivered"},{"mapping_verified",false},{"undo_verified",false}});
        finish({});
    }
    void finish(const QString &reason) {
        if(!active)return;
        active=false;deadline.stop();planning.cancel();qApp->removeEventFilter(this);
        for(auto connection:selectionWatches)disconnect(connection);
        selectionWatches.clear();
        if(!reason.isEmpty()) {
            if(channelMenu)channelMenu->close();
            if(root)root->close();
            logEvent("command.audio-channel.native-rejected",{{"reason",reason}});
        }
        timeline=nullptr;scene=nullptr;item=nullptr;root=nullptr;channelMenu=nullptr;
    }
    QPointer<QGraphicsView> timeline,pendingView;
    QPointer<QGraphicsScene> scene;
    QGraphicsItem *item=nullptr; // Compared only while retained scene owns selection.
    QPointer<QMenu> root,channelMenu;
    QTimer deadline;
    AudioChannelPlanRequest planning;
    QList<QMetaObject::Connection> selectionWatches;
    int direction=0,expectedCurrent=0,plannedTarget=0,sourceCount=0;
    bool active=false,sending=false,issued=false;
};
}
