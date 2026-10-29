// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QMetaMethod>
#include <QSet>
#include "diagnostics.h"
#include "nativeaudiochannelchoice.h"
#include "nativemenupreparationprobe.h"
#include "nativetimelineselection.h"
namespace Unbound {
// Read-only, bounded evidence for the native channel-menu/undo investigation.
// Uses existing Show observations; never prepares, opens or invokes a menu.
class NativeAudioChannelProbe : public QObject {
public:
    explicit NativeAudioChannelProbe(QObject *owner):QObject(owner),preparation(this) {}
    void inspectPreparation(QMenu *menu) {
        if(!menu || menu->property("unboundOwnMenu").toBool())return;
        bool relevant=label(menu->title())=="Audio Channel";
        for(auto action:menu->actions())if(action->menu() && label(action->text())=="Audio Channel") {
            relevant=true;preparation.observe(action->menu());
        }
        if(relevant)preparation.observe(menu);
    }
    void inspectShownMenu(QMenu *menu) {
        inspectPreparation(menu);
        if(!menu || captures>=32 || menu->property("unboundOwnMenu").toBool())return;
        QList<QMenu *> channels;
        if(label(menu->title())=="Audio Channel")channels.append(menu);
        for(auto action:menu->actions())
            if(action->menu() && label(action->text())=="Audio Channel")channels.append(action->menu());
        for(auto channel:channels) {
            if(captures++>=32)break;
            if(!clipMenuInspected) {
                clipMenuInspected=true;
                // Inspect the actual native Clip menu's connections, not just
                // a method whose name suggests it might prepare that menu.
                for(auto widget:QApplication::allWidgets())if(auto native=qobject_cast<QMenu *>(widget))
                    if(!native->property("unboundOwnMenu").toBool() && label(native->title())=="Clip") {
                        preparation.inspectConnections(native,"native-clip-menu");preparation.observe(native);
                    }
            }
            QJsonArray rows,ancestors;
            for(auto action:channel->actions().mid(0,64)) {
                rows.append(actionState(action));
                if(tracked.contains(action))continue;
                tracked.insert(action);
                connect(action,&QObject::destroyed,this,[this,action]{tracked.remove(action);});
                connect(action,&QAction::triggered,this,[this,action]{
                    if(triggers++>=64)return;
                    logEvent("audio.native-menu.triggered",{{"action",actionState(action)},
                        {"observation","qt-action-signal-only"},{"undo_verified",false}});
                });
            }
            auto focus=QApplication::focusWidget();
            for(int depth=0;focus && depth<12;++depth,focus=focus->parentWidget()) {
                QJsonArray methods;
                const auto meta=focus->metaObject();
                for(int i=0;i<meta->methodCount() && methods.size()<64;++i) {
                    const auto method=meta->method(i);const auto signature=method.methodSignature();
                    const auto lower=signature.toLower();
                    if(lower.contains("audio") || lower.contains("channel") || lower.contains("contextmenu"))
                        methods.append(QJsonObject{{"signature",QString::fromLatin1(signature)},
                            {"type",int(method.methodType())},{"access",int(method.access())}});
                }
                ancestors.append(QJsonObject{{"class",meta->className()},{"name",focus->objectName()},{"methods",methods}});
            }
            QJsonArray sceneOwners;
            auto timeline=NativeTimelineSelection::focusedView();
            if(timeline) {
                const auto selection=NativeTimelineSelection::inspect(timeline);
                QList<QGraphicsScene *> owners;
                if(selection.scene)owners.append(selection.scene);
                if(timeline->scene() && !owners.contains(timeline->scene()))owners.append(timeline->scene());
                for(auto scene:owners)preparation.inspectConnections(scene,"timeline-scene");
                for(auto scene:owners)sceneOwners.append(QJsonObject{
                    {"scene",NativeMenuPreparationProbe::describe(scene)},
                    {"owners",NativeMenuPreparationProbe::owners(scene)}});
            }
            const auto previous=NativeAudioChannelChoice::inspect(channel,-1);
            const auto next=NativeAudioChannelChoice::inspect(channel,1);
            logEvent("audio.native-menu.inventory",{{"menu_class",channel->metaObject()->className()},
                {"menu_name",channel->objectName()},{"parent_class",channel->parent()?channel->parent()->metaObject()->className():""},
                {"parent_name",channel->parent()?channel->parent()->objectName():QString()},
                {"actions",rows},{"focus_ancestry",ancestors},{"scene_owners",sceneOwners},{"read_only",true},
                {"previous_target",previous.target},{"next_target",next.target},
                {"choice_rejection",next.reason},{"dispatch_verified",false}});
        }
    }
private:
    NativeMenuPreparationProbe preparation;bool clipMenuInspected=false;
    int captures=0,triggers=0;QSet<QAction *> tracked;
    static QString label(QString value){value.remove('&');return value.trimmed();}
    static QJsonObject actionState(QAction *action) {
        const auto data=action->data();QJsonValue scalar;
        // Do not dereference or serialize native handles/custom payloads.
        if(data.type()==QVariant::Int)scalar=data.toInt();
        else if(data.type()==QVariant::UInt)scalar=double(data.toUInt());
        else if(data.type()==QVariant::Bool)scalar=data.toBool();
        else if(data.type()==QVariant::String)scalar=data.toString().left(128);
        return {{"name",action->objectName()},{"label",action->text().left(128)},
            {"enabled",action->isEnabled()},{"checkable",action->isCheckable()},{"checked",action->isChecked()},
            {"data_type",data.typeName()?data.typeName():""},{"scalar_data",scalar},
            {"parent_class",action->parent()?action->parent()->metaObject()->className():""},
            {"parent_name",action->parent()?action->parent()->objectName():QString()}};
    }
};
}
