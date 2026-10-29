// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointer>
#include <QSet>
namespace Unbound {
// A bounded read-only selection snapshot within one focused Timeline view.
// Proxy-embedded tracks views need not appear in the outer widget ancestry.
struct NativeTimelineSelection {
    QPointer<QGraphicsScene> scene;
    QPointer<QGraphicsView> view;
    QGraphicsItem *item=nullptr;
    QList<QPointer<QGraphicsScene>> scenes;
    QString reason;
    static QGraphicsView *viewFor(QWidget *receiver) {
        for(auto widget=receiver;widget;widget=widget->parentWidget())
            if(widget->objectName()=="m_TiTimelineView")return qobject_cast<QGraphicsView *>(widget);
        return nullptr;
    }
    static QGraphicsView *focusedView() { return viewFor(QApplication::focusWidget()); }
    static NativeTimelineSelection inspect(QGraphicsView *root) {
        NativeTimelineSelection result;
        QList<QGraphicsView *> pending{root};QSet<QGraphicsView *> visited;
        QSet<QGraphicsScene *> inspected;
        int itemBudget=4096;
        while(!pending.isEmpty()) {
            auto view=pending.takeFirst();
            if(!view || visited.contains(view))continue;
            if(visited.size()>=64){result.reason="native-timeline-view-limit";return result;}
            visited.insert(view);
            if(!view->isVisible() || !view->scene())continue;
            for(auto child:view->findChildren<QGraphicsView *>())pending.append(child);
            auto scene=view->scene();
            if(inspected.contains(scene))continue;
            inspected.insert(scene);result.scenes.append(scene);
            for(auto selected:scene->selectedItems()) {
                if(result.item){result.reason="single-native-selected-item-required";return result;}
                if(!selected->isVisible() || !selected->isEnabled()) {
                    result.reason="native-selected-item-unavailable";return result;
                }
                result.item=selected;result.scene=scene;result.view=view;
            }
            // Follow only actual Qt proxy ownership; never infer clip identity
            // from coordinates, labels or Resolve's private handle payloads.
            const auto items=scene->items();
            if(items.size()>itemBudget){result.reason="native-timeline-item-limit";return result;}
            itemBudget-=items.size();
            for(auto item:items)if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
                if(!proxy->isVisible() || !proxy->widget())continue;
                if(auto nested=qobject_cast<QGraphicsView *>(proxy->widget()))pending.append(nested);
                for(auto nested:proxy->widget()->findChildren<QGraphicsView *>())pending.append(nested);
            }
        }
        if(!result.item)result.reason="single-native-selected-item-required";
        return result;
    }
};
}
