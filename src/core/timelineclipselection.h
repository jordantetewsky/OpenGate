// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPointer>
#include <QSet>
namespace Unbound {
// Resolve selected native clip containers through the existing nested views.
// No assignment, paint, inspection, project persistence or command ownership.
class TimelineClipSelection {
public:
    struct Clip { QPointer<QGraphicsView> surface;QPointer<QGraphicsObject> object; };
    static QGraphicsObject *container(QGraphicsItem *item) {
        for (auto p=item;p;p=p->parentItem())
            if (auto object=p->toGraphicsObject())
                if (object->inherits("UiTiClipContainer"))return object;
        return nullptr;
    }
    static QList<QGraphicsView *> views(QGraphicsView *root) {
        QList<QGraphicsView *> result,pending;
        if(root)pending.append(root);
        while(!pending.isEmpty()&&result.size()<32) {
            auto next=pending.takeFirst();
            if(!next||result.contains(next))continue;
            result.append(next);
            if(auto scene=next->scene())for(auto item:scene->items())
                if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item))
                    if(auto nested=qobject_cast<QGraphicsView *>(proxy->widget()))pending.append(nested);
        }
        return result;
    }
    static QVector<Clip> selected(QGraphicsView *root) {
        QVector<Clip> result;QSet<QGraphicsObject *> seen;
        for(auto surface:views(root))if(surface->scene())
            for(auto item:surface->scene()->selectedItems())
                if(auto clip=container(item))if(!seen.contains(clip)) {
                    seen.insert(clip);result.append({surface,clip});
                }
        return result;
    }
};
}
