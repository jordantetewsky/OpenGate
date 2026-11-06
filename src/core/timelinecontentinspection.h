// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsEffect>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QSet>
#include <QPointer>
#include <QApplication>
#include <QThread>
#include "diagnostics.h"
#include "../platform/timelineclipidentityinspection.h"
namespace Unbound {
// Bounded graphics topology with an opt-in platform identity diagnostic.
// Metadata getter inspection is separately restricted to disposable SDK IDs.
class TimelineContentInspection {
public:
    // Compact backend/cache facts for native parity experiments. This does not
    // render widgets, invalidate caches, attach effects or choose an adapter.
    static QJsonObject captureRenderState(QGraphicsView *view) {
        QJsonObject result{{"read_only",true},{"exists",view!=nullptr}};
        if(!view || !qApp || QThread::currentThread()!=qApp->thread())return result;
        result["class"]=view->metaObject()->className();
        result["name"]=view->objectName();
        result["view_cache_mode"]=int(view->cacheMode());
        result["viewport_update_mode"]=int(view->viewportUpdateMode());
        result["device_pixel_ratio"]=view->devicePixelRatioF();
        result["view_effect"]=effectState(view->graphicsEffect());
        if(auto viewport=view->viewport())
            result["viewport"]=QJsonObject{{"class",viewport->metaObject()->className()},
                {"native",viewport->testAttribute(Qt::WA_NativeWindow)},
                {"paint_on_screen",viewport->testAttribute(Qt::WA_PaintOnScreen)},
                {"open_gl_widget",viewport->inherits("QOpenGLWidget")},
                {"legacy_gl_widget",viewport->inherits("QGLWidget")},
                {"effect",effectState(viewport->graphicsEffect())}};
        QJsonArray ancestry;QSet<QGraphicsView *> visited;
        auto surface=view;bool truncated=false;
        for(int depth=0;surface; ++depth) {
            if(depth>=16 || visited.contains(surface)){truncated=true;break;}
            visited.insert(surface);
            auto host=static_cast<QWidget *>(surface);
            while(host && !host->graphicsProxyWidget())host=host->parentWidget();
            if(!host)break;
            auto proxy=host->graphicsProxyWidget();
            const auto parents=proxy->scene()?proxy->scene()->views():QList<QGraphicsView *>{};
            ancestry.append(QJsonObject{{"proxy_cache_mode",int(proxy->cacheMode())},
                {"proxy_effect",effectState(proxy->graphicsEffect())},
                {"host_class",host->metaObject()->className()},
                {"host_effect",effectState(host->graphicsEffect())},
                {"scene_view_count",parents.size()}});
            if(parents.size()!=1){truncated=true;break;}
            surface=parents.first();
        }
        result["proxy_ancestry"]=ancestry;result["ancestry_truncated"]=truncated;
        return result;
    }
    static QJsonObject captureClipMembers(QGraphicsObject *container, bool inspectNativeIdentity = false) {
        const bool gui = qApp && QThread::currentThread() == qApp->thread();
        QJsonObject result{{"read_only", true}, {"gui_thread", gui},
                           {"persistent_identity_verified", false}, {"private_access", false},
                           {"private_calls", false}, {"project_writes", false}};
        if (!container || !gui)
            return result;
        QJsonArray members;
        const auto children = container->childItems();
        bool truncated = children.size() > 64;
        auto pending = children.mid(0, 64);
        while (!pending.isEmpty() && members.size() < 64) {
            auto item = pending.takeFirst();
            auto object = dynamic_cast<QObject *>(item);
            QJsonObject member{{"graphics_item", QString::number(quintptr(item), 16)},
                               {"graphics_parent", QString::number(quintptr(item->parentItem()), 16)},
                               {"scene", QString::number(quintptr(item->scene()), 16)},
                               {"to_graphics_object", item->toGraphicsObject() != nullptr},
                               {"qobject_cross_cast", object != nullptr},
                               {"graphics_cache_mode",int(item->cacheMode())},
                               {"graphics_effect",effectState(item->graphicsEffect())}};
            if (object) {
                QPointer<QObject> alive = object;
                member["qobject"] = QString::number(quintptr(object), 16);
                member["class"] = object->metaObject()->className();
                member["native_timeline_item"] = object->inherits("UiTiItem");
                member["qobject_parent"] = QString::number(quintptr(object->parent()), 16);
                member["alive"] = !alive.isNull();
                if (member["native_timeline_item"].toBool()) {
                    const auto identity = inspectTimelineClipIdentity(object, inspectNativeIdentity);
                    member["identity_probe"] = identity;
                    if (identity["bounded_memory_reads"].toInt() > 0)
                        result["private_access"] = true;
                }
            }
            members.append(member);
            // Bound both visited nodes and the pending tree, including wide trees.
            for (auto child : item->childItems()) {
                if (pending.size() >= 64) {
                    truncated = true;
                    break;
                }
                pending.append(child);
            }
        }
        result["container_graphics_item"] =
            QString::number(quintptr(static_cast<QGraphicsItem *>(container)), 16);
        result["container_qobject"] = QString::number(quintptr(container), 16);
        result["members"] = members;
        result["truncated"] = truncated || !pending.isEmpty();
        return result;
    }
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
    static QJsonObject effectState(QGraphicsEffect *effect) {
        return effect ? QJsonObject{{"present",true},{"class",effect->metaObject()->className()},
                                    {"enabled",effect->isEnabled()}}
                      : QJsonObject{{"present",false}};
    }
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
