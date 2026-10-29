// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QMetaMethod>
#include <QDebug>
#include <QPointer>
#include <QSet>
#include "diagnostics.h"
namespace Unbound {
// Observe preparation signals and menu lifetime, never emit or invoke them.
class NativeMenuPreparationProbe : public QObject {
public:
    explicit NativeMenuPreparationProbe(QObject *owner):QObject(owner){}
    void observe(QMenu *menu) {
        if(!menu || menu->property("unboundOwnMenu").toBool() || watched.contains(menu) || inventories>=64)return;
        watched.insert(menu);const int id=++inventories;
        record(menu,id,"first-observed");
        connect(menu,&QMenu::aboutToShow,this,[this,menu,id]{if(transitions++<128)record(menu,id,"about-to-show");});
        connect(menu,&QMenu::aboutToHide,this,[this,menu,id]{if(transitions++<128)record(menu,id,"about-to-hide");});
        const auto className=QString::fromLatin1(menu->metaObject()->className());
        connect(menu,&QObject::destroyed,this,[this,menu,id,className]{
            watched.remove(menu);
            if(transitions++<128)logEvent("audio.native-menu.preparation",{{"menu_id",id},{"phase","destroyed"},
                {"class",className},{"read_only",true}});
        });
    }
    void inspectConnections(QObject *object,const QString &role) {
        if(!object || dumped.contains(object) || connectionDumps>=16)return;
        dumped.insert(object);const int id=++connectionDumps;
        connect(object,&QObject::destroyed,this,[this,object]{dumped.remove(object);});
        logEvent("audio.native-menu.connections",{{"connection_dump_id",id},{"role",role},
            {"object",describe(object)},{"output","Qt debug stream"},{"read_only",true},
            {"native_operation_invoked",false},{"functor_receivers_may_be_opaque",true}});
        // Use Qt's own public introspection, not private connection structures
        // or a process-wide message handler. Capture its normal debug stream.
        qDebug("OpenGate native connections BEGIN id=%d role=%s",id,qPrintable(role));
        object->dumpObjectInfo();
        qDebug("OpenGate native connections END id=%d",id);
    }
    static QJsonArray owners(QObject *object) {
        QJsonArray result;
        for(auto owner=object?object->parent():nullptr;owner && result.size()<12;owner=owner->parent())
            result.append(describe(owner));
        return result;
    }
    static QJsonObject describe(QObject *object) {
        if(!object)return {{"exists",false}};
        QJsonArray methods;
        const auto meta=object->metaObject();
        for(int i=0;i<meta->methodCount() && methods.size()<96;++i) {
            const auto method=meta->method(i);const auto signature=method.methodSignature();const auto lower=signature.toLower();
            if(lower.startsWith("aboutto") || lower.contains("menu") || lower.contains("audio") || lower.contains("channel") || lower.contains("selection"))
                methods.append(QJsonObject{{"signature",QString::fromLatin1(signature)},
                    {"kind",int(method.methodType())},{"access",int(method.access())}});
        }
        QJsonObject result{{"exists",true},{"class",meta->className()},{"name",object->objectName()},{"methods",methods}};
        if(auto menu=qobject_cast<QMenu *>(object))result["title"]=menu->title();
        return result;
    }
private:
    QSet<QMenu *> watched;QSet<QObject *> dumped;int inventories=0,transitions=0,connectionDumps=0;
    void record(QMenu *menu,int id,const QString &phase) {
        if(phase=="about-to-show") {
            inspectConnections(menu,"audio-menu");
            if(menu->title().trimmed()=="Audio Channel")for(auto action:menu->actions().mid(0,8))
                inspectConnections(action,"audio-channel-action");
        }
        QJsonArray associated,actions;
        for(auto action:menu->actions().mid(0,64))actions.append(QJsonObject{
            {"name",action->objectName()},{"label",action->text().left(128)},
            {"submenu",action->menu()?action->menu()->title():QString()},{"enabled",action->isEnabled()}});
        for(auto widget:menu->menuAction()->associatedWidgets()) {
            if(associated.size()>=16)break;
            associated.append(describe(widget));
        }
        logEvent("audio.native-menu.preparation",{{"menu_id",id},{"phase",phase},
            {"menu",describe(menu)},{"owners",owners(menu)},{"associated_widgets",associated},
            {"actions",actions},{"visible",menu->isVisible()},{"hidden",menu->isHidden()},{"read_only",true},
            {"preparation_invoked",false},{"action_invoked",false}});
    }
};
}
