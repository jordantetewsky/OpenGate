// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QJsonArray>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QPointer>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QLayout>
#include "diagnostics.h"
namespace Unbound {
// Read-only capability inventory for lazy page controls. No layout hints, property
// getters, method invocation, input, reparenting or project/control text is read.
// This is evidence for page-specific adapters, never permission to move a widget.
class PagePanelInventory : public QObject {
public:
    enum class Scope { Pages, MainWindow };
    explicit PagePanelInventory(QObject *owner,Scope scope=Scope::Pages):QObject(owner),scope(scope) {
        qApp->installEventFilter(this);
        QTimer::singleShot(0,this,[this] {
            for(auto window:QApplication::topLevelWidgets())if(window->objectName()=="UiMainWindow") {
                if(this->scope==Scope::MainWindow) { if(window->isVisible())schedule(window);continue; }
                for(auto page:window->findChildren<QWidget *>())
                    if(page->objectName().startsWith("WStackPage_") && page->isVisible())schedule(page);
            }
        });
    }
    ~PagePanelInventory() override {if(qApp)qApp->removeEventFilter(this);}
    static QString identity(QObject *object) {return object?QString::number(quintptr(object),16):QString();}
    static QJsonObject capture(QWidget *page,int maximum=2048) {
        QJsonArray nodes;QList<QObject *> pending;if(page)pending.append(page);
        maximum=qBound(1,maximum,4096);
        while(!pending.isEmpty() && nodes.size()<maximum) {
            auto object=pending.takeFirst();
            QJsonObject node{{"id",identity(object)},{"parent_id",identity(object->parent())},
                {"name",object->objectName()},{"class",object->metaObject()->className()}};
            if(auto widget=qobject_cast<QWidget *>(object)) {
                const auto frame=widget->geometry();const auto minimum=widget->minimumSize();const auto maximum=widget->maximumSize();
                node["widget"]=true;node["visible"]=widget->isVisible();node["hidden"]=widget->isHidden();
                node["window"]=widget->isWindow();node["native"]=widget->testAttribute(Qt::WA_NativeWindow);
                node["paint_on_screen"]=widget->testAttribute(Qt::WA_PaintOnScreen);
                node["geometry"]=QJsonObject{{"x",frame.x()},{"y",frame.y()},{"width",frame.width()},{"height",frame.height()}};
                node["minimum"]=QJsonArray{minimum.width(),minimum.height()};node["maximum"]=QJsonArray{maximum.width(),maximum.height()};
                node["layout_id"]=identity(widget->layout());
            }
            QJsonArray methods,properties;
            const auto meta=object->metaObject();
            for(int i=0;i<meta->methodCount() && methods.size()<32;++i) {
                const auto method=meta->method(i);const auto signature=method.methodSignature();const auto lower=signature.toLower();
                if(lower.contains("scale") || lower.contains("zoom") || lower.contains("panel") || lower.contains("dock") || lower.contains("track") || lower.contains("ruler"))
                    methods.append(QJsonObject{{"signature",QString::fromLatin1(signature)},{"type",int(method.methodType())},{"access",int(method.access())}});
            }
            for(int i=0;i<meta->propertyCount() && properties.size()<32;++i) {
                const auto property=meta->property(i);const auto name=QByteArray(property.name());const auto lower=name.toLower();
                if(lower.contains("scale") || lower.contains("zoom") || lower.contains("track") || lower.contains("ruler"))
                    properties.append(QJsonObject{{"name",QString::fromLatin1(name)},{"type",QString::fromLatin1(property.typeName())},{"writable",property.isWritable()}});
            }
            if(!methods.isEmpty())node["candidate_methods"]=methods;
            if(!properties.isEmpty())node["candidate_properties"]=properties;
            nodes.append(node);
            // Parent ids preserve unnamed/duplicate-name structure. Never include
            // detached windows belonging to another owner in this page inventory.
            for(auto child:object->children())if(auto widget=qobject_cast<QWidget *>(child)) {
                if(!widget->isWindow())pending.append(widget);
            } else if(qobject_cast<QLayout *>(child))pending.append(child);
        }
        return {{"page_widget",page?page->objectName():QString()},{"nodes",nodes},{"truncated",!pending.isEmpty()},
            {"limit",maximum},{"read_only",true},{"qt",qVersion()},{"session",diagnosticSessionId()},{"build",UNBOUND_BUILD_ID}};
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()!=QEvent::Show && event->type()!=QEvent::ChildAdded)return false;
        if(auto widget=qobject_cast<QWidget *>(object))
            for(auto page=widget;page;page=page->parentWidget())
                if(matches(page)){if(page->isVisible())schedule(page);break;}
        return false;
    }
private:
    bool matches(QWidget *widget) const {
        return scope==Scope::MainWindow?widget->objectName()=="UiMainWindow":widget->objectName().startsWith("WStackPage_");
    }
    void schedule(QWidget *page) {
        if(queued.contains(page) || captures.value(page)>=4)return;
        if(!captures.contains(page)) {
            captures[page]=0;
            connect(page,&QObject::destroyed,this,[this,page] {queued.remove(page);captures.remove(page);});
        }
        queued.insert(page);QPointer<QWidget> safe(page);
        QTimer::singleShot(750,this,[this,safe,page] {
            queued.remove(page);if(!safe || !safe->isVisible())return;
            const auto count=++captures[page];const auto inventory=capture(safe);
            const auto directory=diagnosticDirectory()+(scope==Scope::MainWindow?"/WindowInventory":"/PageInventory");
            QDir().mkpath(directory);
            auto name=safe->objectName();name.replace('/','_');name.replace('\\','_');
            const auto path=directory+"/"+diagnosticSessionId()+"-"+name+"-"+QString::number(count)+".json";
            QSaveFile file(path);bool written=false;
            if(file.open(QIODevice::WriteOnly)) {const auto bytes=QJsonDocument(inventory).toJson(QJsonDocument::Compact);written=file.write(bytes)==bytes.size() && file.commit();}
            const auto retained=QDir(directory).entryList({scope==Scope::MainWindow?"*-UiMainWindow-*.json":"*-WStackPage_*-*.json"},QDir::Files,QDir::Time);
            for(int i=32;i<retained.size();++i)QFile::remove(directory+"/"+retained[i]);
            logEvent("capability.page.inventory",{{"page_widget",safe->objectName()},{"path",path},{"success",written},
                {"nodes",inventory["nodes"].toArray().size()},{"truncated",inventory["truncated"]},{"read_only",true}});
            // One additional settled capture covers lazily initialized page content.
            if(count==1)QTimer::singleShot(1500,this,[this,safe] {if(safe && safe->isVisible())schedule(safe);});
        });
    }
    QSet<QWidget *> queued;QHash<QWidget *,int> captures;
    Scope scope;
};
}
