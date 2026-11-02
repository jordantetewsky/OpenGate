// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QGraphicsEffect>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include "clipfillpaintdevice.h"
#include "diagnostics.h"

namespace Unbound {
// A temporary visual experiment, never a saved clip color. Native clip objects,
// scene selection, input and commands remain native. Existing effects are skipped.
class TimelineClipColorPreview : public QObject {
public:
    TimelineClipColorPreview(QGraphicsView *view,QWidget *scope):QObject(scope),view(view),scope(scope) {
        if(qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_RED_PREVIEW")==1)qApp->installEventFilter(this);
    }
    ~TimelineClipColorPreview() override {
        if(qApp)qApp->removeEventFilter(this);clear();
        for(auto action:actions)if(action)delete action;
    }
    int applySelected(QColor color) {
        if(!color.isValid())return 0;
        for(int i=clips.size()-1;i>=0;--i)if(!clips[i].clip || !clips[i].effect)clips.removeAt(i);
        int count=0;QSet<QGraphicsObject *> seen;
        for(auto scene:scenes())for(auto item:scene->selectedItems()) {
            QGraphicsObject *clip=nullptr;
            for(auto p=item;p;p=p->parentItem())if(auto object=p->toGraphicsObject())
                if(object->inherits("UiTiClipContainer")){clip=object;break;}
            if(!clip || seen.contains(clip))continue;seen.insert(clip);
            auto effect=dynamic_cast<Effect *>(clip->graphicsEffect());
            if(clip->graphicsEffect() && (!effect || effect->owner!=this))continue;
            if(!effect){effect=new Effect(this);clip->setGraphicsEffect(effect);clips.append({clip,effect});}
            effect->color=color;effect->reported=false;effect->update();++count;
        }
        logEvent("clip-color.preview.requested",{{"clips",count},{"color",color.name()},
            {"temporary",true},{"project_writes",false}});
        return count;
    }
    void clear() {
        for(const auto &lease:clips)if(lease.clip && lease.effect && lease.clip->graphicsEffect()==lease.effect)
            lease.clip->setGraphicsEffect(nullptr);
        clips.clear();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()!=QEvent::Show)return false;
        auto menu=qobject_cast<QMenu *>(object);
        if(!menu || menu->objectName()!="markSetClipColor" || !scope)return false;
        for(int i=actions.size()-1;i>=0;--i)if(!actions[i])actions.removeAt(i);
        auto focus=QApplication::focusWidget();const bool focused=focus && (focus==scope || scope->isAncestorOf(focus));
        bool found=false;
        for(auto action:menu->actions())if(action->property("openGateClipPreviewOwner").value<QObject *>()==this) {
            action->setVisible(focused);found=true;
        }
        if(!focused || found)return false;
        QList<QAction *> added{menu->addSeparator()};
        auto red=menu->addAction("Preview selected clips in Red (temporary)");red->setObjectName("openGateRedClipPreview");
        connect(red,&QAction::triggered,this,[this]{applySelected(QColor("#ff0000"));});
        auto reset=menu->addAction("Clear temporary clip-color preview");
        connect(reset,&QAction::triggered,this,[this]{clear();});
        added<<red<<reset;
        for(auto action:added){action->setProperty("openGateClipPreviewOwner",QVariant::fromValue<QObject *>(this));actions.append(action);}
        return false;
    }
private:
    class Effect : public QGraphicsEffect {
    public:
        explicit Effect(QObject *owner):owner(owner){}QPointer<QObject> owner;QColor color;bool reported=false;
    protected:
        void draw(QPainter *target) override {
            // Inspect commands, then replace only the largest chromatic rectangle.
            // Both passes use the source's actual text runs; no image tint or
            // serialized/re-shaped text. The extra pass is experimental overhead.
            const ClipFillPaintDevice::State state(*target);
            ClipFillPaintDevice inspect(nullptr,{},{},target->device());
            {QPainter scan(&inspect);state.apply(scan);drawSource(&scan);}
            const auto fill=inspect.largestFill();
            ClipFillPaintDevice device(target,fill.color,color);
            {QPainter output(&device);state.apply(output);drawSource(&output);}
            if(!reported){reported=true;logEvent("clip-color.preview.paint",{
                {"source_fill",fill.color.isValid()?fill.color.name():QString()},
                {"target",color.name()},{"candidate_area",fill.area},
                {"changed_fill_commands",device.changedFillCommands()},{"temporary",true}});}
        }
    };
    struct Lease {QPointer<QGraphicsObject> clip;QPointer<Effect> effect;};
    QPointer<QGraphicsView> view;QPointer<QWidget> scope;QVector<Lease> clips;QVector<QPointer<QAction>> actions;
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
