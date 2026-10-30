// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QSet>
#include "diagnostics.h"
namespace Unbound {
// Bounded read-only graphics topology; never invokes native methods or reads text.
class TimelineContentInspection {
public:
    static QJsonObject capture(QWidget *timeline) {
        QJsonArray views;QSet<QGraphicsView *> seen;int itemsLeft=256;
        auto pending=timeline?timeline->findChildren<QGraphicsView *>():QList<QGraphicsView *>{};
        while(!pending.isEmpty() && views.size()<24) {
            auto view=pending.takeFirst();if(seen.contains(view))continue;seen.insert(view);
            auto scene=view->scene();QJsonArray items;
            if(scene)for(auto item:scene->items()) {
                if(itemsLeft<=0)break;
                --itemsLeft;
                const auto object=item->toGraphicsObject();
                QJsonObject state{{"type",item->type()},{"rect",rectangle(item->sceneBoundingRect())},
                    {"visible",item->isVisible()},{"flags",int(item->flags())}};
                if(object)state["interface"]=interface(object);
                if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item))if(auto widget=proxy->widget()) {
                    state["widget"]=interface(widget);
                    if(auto nested=qobject_cast<QGraphicsView *>(widget))pending.append(nested);
                    pending.append(widget->findChildren<QGraphicsView *>());
                }
                items.append(state);
            }
            const auto matrix=view->transform();auto viewport=view->viewport();
            views.append(QJsonObject{{"interface",interface(view)},{"scene",scene?interface(scene):QJsonObject{}},
                {"geometry",rectangle(view->geometry())},{"scene_rect",rectangle(view->sceneRect())},
                {"transform",QJsonArray{matrix.m11(),matrix.m12(),matrix.m21(),matrix.m22(),matrix.dx(),matrix.dy()}},
                {"viewport_class",viewport?viewport->metaObject()->className():""},
                {"native_viewport",viewport&&viewport->testAttribute(Qt::WA_NativeWindow)},
                {"paint_on_screen",viewport&&viewport->testAttribute(Qt::WA_PaintOnScreen)},
                {"items",items}});
        }
        return {{"views",views},{"truncated",!pending.isEmpty()||itemsLeft==0},{"read_only",true}};
    }
private:
    static QJsonArray rectangle(const QRectF &rect) {return {rect.x(),rect.y(),rect.width(),rect.height()};}
    static QJsonObject interface(QObject *object) {
        QJsonArray classes,methods,properties;
        for(auto meta=object->metaObject();meta;meta=meta->superClass())classes.append(meta->className());
        const auto meta=object->metaObject();
        for(int i=0;i<meta->methodCount() && methods.size()<48;++i) {
            const auto method=meta->method(i);const auto name=method.methodSignature().toLower();
            if(name.contains("width")||name.contains("scale")||name.contains("resize")||name.contains("scroll")||name.contains("splitter"))
                methods.append(QJsonObject{{"signature",QString::fromLatin1(method.methodSignature())},{"type",int(method.methodType())},{"access",int(method.access())}});
        }
        for(int i=0;i<meta->propertyCount() && properties.size()<24;++i) {
            const auto property=meta->property(i);const auto name=QByteArray(property.name()).toLower();
            if(name.contains("width")||name.contains("scale")||name.contains("scroll"))
                properties.append(QJsonObject{{"name",property.name()},{"type",property.typeName()},{"writable",property.isWritable()}});
        }
        return {{"id",QString::number(quintptr(object),16)},{"name",object->objectName()},
            {"classes",classes},{"candidate_methods",methods},{"candidate_properties",properties}};
    }
};
}
