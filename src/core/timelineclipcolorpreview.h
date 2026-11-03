// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QGraphicsEffect>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPointer>
#include <QSet>
#include "clipfillgraphicseffect.h"
#include "diagnostics.h"

namespace Unbound {
// A temporary visual experiment, never a saved clip color. Native clip objects,
// scene selection, input and commands remain native. Existing effects are skipped.
class TimelineClipColorPreview : public QObject {
public:
    TimelineClipColorPreview(QGraphicsView *view,QObject *owner):QObject(owner),view(view) {}
    ~TimelineClipColorPreview() override {clear();}
    bool hasSelection() const {
        for(auto scene:scenes())for(auto item:scene->selectedItems())if(container(item))return true;
        return false;
    }
    int applySelected(QColor color) {
        if(!color.isValid())return 0;
        for(int i=clips.size()-1;i>=0;--i)if(!clips[i].clip || !clips[i].effect)release(i);
        int count=0;QSet<QGraphicsObject *> seen;
        for(auto scene:scenes())for(auto item:scene->selectedItems()) {
            auto clip=container(item);
            if(!clip || seen.contains(clip))continue;seen.insert(clip);
            auto effect=dynamic_cast<ClipFillGraphicsEffect *>(clip->graphicsEffect());
            if(clip->graphicsEffect() && (!effect || effect->owner()!=this))continue;
            if(!effect){auto cache=new ClipDeviceCacheLease(this,clip);effect=new ClipFillGraphicsEffect(this,cache);cache->bind(effect);
                const QPointer<QGraphicsObject> alive=clip;const QPointer<ClipFillGraphicsEffect> paint=effect;
                effect->unsupported=[alive,paint]{if(alive && paint && alive->graphicsEffect()==paint) {
                    logEvent("clip-color.preview.unsupported",{{"native_restored",true},{"project_writes",false}});
                    alive->setGraphicsEffect(nullptr);}};
                clip->setGraphicsEffect(effect);clips.append({clip,effect,cache});}
            effect->setColor(color);++count;
        }
        logEvent("clip-color.preview.requested",{{"clips",count},{"color",color.name()},
            {"temporary",true},{"project_writes",false}});
        return count;
    }
    void clear() {
        while(!clips.isEmpty())release(clips.size()-1);
    }
    void clearSelected() {
        QSet<QGraphicsObject *> selected;
        for(auto scene:scenes())for(auto item:scene->selectedItems())if(auto clip=container(item))selected.insert(clip);
        for(int i=clips.size()-1;i>=0;--i)if(selected.contains(clips[i].clip))release(i);
    }
private:
    struct Lease {QPointer<QGraphicsObject> clip;QPointer<ClipFillGraphicsEffect> effect;QPointer<ClipDeviceCacheLease> cache;};
    QPointer<QGraphicsView> view;QVector<Lease> clips;
    void release(int index) {
        const auto lease=clips.takeAt(index);
        if(lease.clip && lease.effect && lease.clip->graphicsEffect()==lease.effect)lease.clip->setGraphicsEffect(nullptr);
        delete lease.cache;
    }
    static QGraphicsObject *container(QGraphicsItem *item) {
        for(auto p=item;p;p=p->parentItem())if(auto object=p->toGraphicsObject())
            if(object->inherits("UiTiClipContainer"))return object;
        return nullptr;
    }
    QList<QGraphicsScene *> scenes() const {
        QList<QGraphicsScene *> result;QList<QGraphicsView *> pending;if(view)pending.append(view);
        QSet<QGraphicsView *> visited;
        while(!pending.isEmpty() && visited.size()<32) {
            auto next=pending.takeFirst();if(!next || visited.contains(next))continue;visited.insert(next);
            auto scene=next->scene();if(!scene || result.contains(scene))continue;result.append(scene);
            for(auto item:scene->items())if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item))
                if(auto nested=qobject_cast<QGraphicsView *>(proxy->widget()))pending.append(nested);
        }
        return result;
    }
};
}
