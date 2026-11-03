// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsObject>
#include <QGraphicsEffect>
#include <QPointer>
#include <QSet>
#include <QVector>

namespace Unbound {
// Reversibly expose device-cached QObject clip paint commands. Item-coordinate
// caches have an unexposed logical size; plain items have no safe weak lifetime.
// Both remain untouched. There is no scene/model/input or color ownership here.
class ClipDeviceCacheLease : public QObject {
public:
    ClipDeviceCacheLease(QObject *owner,QGraphicsObject *root):QObject(owner),root(root){refresh();}
    ~ClipDeviceCacheLease() override {release();}
    void bind(QGraphicsEffect *effect) {
        // An effect can die inside QGraphicsItem's destructor, before QObject's
        // weak pointers clear. Restore next turn, after native deletion.
        connect(effect,&QObject::destroyed,this,[this]{release();},Qt::QueuedConnection);
    }
    void refresh() {
        itemCaches=plainCaches=opacityItems=0;QSet<QGraphicsObject *> current;
        QList<QGraphicsItem *> pending;if(root)pending.append(root);
        int visited=0;
        while(!pending.isEmpty() && ++visited<=2048) {
            auto item=pending.takeLast();pending.append(item->childItems());
            if(!qFuzzyCompare(item->effectiveOpacity(),qreal(1)))++opacityItems;
            auto object=item->toGraphicsObject();
            if(item->cacheMode()==QGraphicsItem::ItemCoordinateCache)++itemCaches;
            if(!object){plainCaches+=item->cacheMode()!=QGraphicsItem::NoCache;continue;}
            current.insert(object);bool known=false;
            for(const auto &entry:items)if(entry==object){known=true;break;}
            if(!known && object->cacheMode()==QGraphicsItem::DeviceCoordinateCache) {
                items.append(object);object->setCacheMode(QGraphicsItem::NoCache);
            }
        }
        for(int i=items.size()-1;i>=0;--i)if(!items[i] || !current.contains(items[i])) {
            restore(items[i]);items.removeAt(i);
        }
    }
    void release(){for(auto item:items)restore(item);items.clear();}
    int leased() const {return items.size();}
    int unsupportedItemCaches() const {return itemCaches;}
    int unsupportedPlainCaches() const {return plainCaches;}
    int unsupportedOpacityItems() const {return opacityItems;}
private:
    QPointer<QGraphicsObject> root;QVector<QPointer<QGraphicsObject>> items;
    int itemCaches=0,plainCaches=0,opacityItems=0;
    static void restore(QGraphicsObject *item) {
        if(item && item->cacheMode()==QGraphicsItem::NoCache)item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
};
}
