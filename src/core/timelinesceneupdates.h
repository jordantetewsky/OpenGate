// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QPaintEvent>
#include <QScrollBar>
#include <QPointer>
#include <QTimer>
#include <functional>
#include "nativecommandactivity.h"

namespace Unbound {
// Observe native damage and geometry independently. Layout work coalesces at an
// event boundary; content changes never introduce a playback/redraw polling timer.
class TimelineSceneUpdates : public QObject {
public:
    TimelineSceneUpdates(QGraphicsView *view,QObject *owner):QObject(owner),view(view) {
        commandSubscription=NativeCommandActivity::shared()->subscribe(this,[this](const QString &source){
            if(stopped || !this->view || !this->view->isVisible())return;
            commandCompleted();if(commandObserved)commandObserved(source);
        });
        layout.setSingleShot(true);paint.setSingleShot(true);
        connect(&paint,&QTimer::timeout,this,[this] {
            if(captureActive())return;
            const auto region=pendingPaint;pendingPaint=QRegion();
            if(this->view && this->view->isVisible() && paintReady)paintReady(region);
        });
        connect(&layout,&QTimer::timeout,this,[this]{
            if(captureActive()){deferredLayout=true;return;}
            if(this->view && this->view->isVisible() && layoutChanged)layoutChanged();
        });
        view->installEventFilter(this);view->viewport()->installEventFilter(this);
        connect(view->horizontalScrollBar(),&QScrollBar::valueChanged,this,[this]{requestLayout();});
        connect(view->verticalScrollBar(),&QScrollBar::valueChanged,this,[this]{requestLayout();});
    }
    ~TimelineSceneUpdates() override { stop(); }
    std::function<void()> layoutChanged,repaint,suspended;
    std::function<bool()> headerMovesWithTime;
    std::function<bool()> capturing;
    std::function<void(const QString &)> commandObserved;
    std::function<void(const QRegion &)> paintReady;
    void schedulePaint(const QRegion &region) {
        if(stopped || !view || !view->isVisible() || region.isEmpty())return;
        pendingPaint+=region;if(pendingPaint.rectCount()>16)pendingPaint=QRegion(pendingPaint.boundingRect());
        if(!captureActive() && !paint.isActive())paint.start(0);
    }
    void captureFinished() {
        if(stopped)return;
        if(deferredLayout){deferredLayout=false;requestLayout();}
        if(!pendingPaint.isEmpty() && !paint.isActive())paint.start(0);
    }
    void geometryInvalidated(){requestLayout();}
    void presented(const QRegion &region) {
        pendingPaint-=region;if(pendingPaint.isEmpty())paint.stop();
    }
    std::function<void(const QList<QRectF> &)> damaged;
    void commandCompleted(){if(!stopped && view && view->isVisible() && damaged)damaged({});}
    void stop() {
        if(auto activity=NativeCommandActivity::existing())activity->unsubscribe(commandSubscription);
        commandSubscription=0;
        stopped=true;layout.stop();paint.stop();pendingPaint=QRegion();
        layoutChanged={};repaint={};suspended={};damaged={};paintReady={};headerMovesWithTime={};commandObserved={};capturing={};
        if(view){view->removeEventFilter(this);if(view->viewport())view->viewport()->removeEventFilter(this);}
        for(auto editor:editors)if(editor)editor->removeEventFilter(this);
        for(const auto &connection:connections)disconnect(connection);
        connections.clear();clearNested();scene=nullptr;
    }
    void refresh() {
        if(stopped || !view || scene==view->scene())return;
        for(const auto &connection:connections)disconnect(connection);
        connections.clear();clearNested();proxies.clear();scene=view->scene();
        if(scene) {
            connections.append(connect(scene,&QGraphicsScene::changed,this,[this](const QList<QRectF> &rects) {
                if(damaged)damaged(rects);
                if(scene && scene->items().size()!=knownItems)requestLayout();
            }));
            connections.append(connect(scene,&QGraphicsScene::sceneRectChanged,this,[this]{requestLayout();}));
        }
        if(damaged)damaged({});requestLayout();
    }
    void geometryObserved() {
        if(!scene)return;
        for(int i=proxies.size()-1;i>=0;--i)if(!proxies[i])proxies.removeAt(i);
        for(int i=nestedViews.size()-1;i>=0;--i)if(!nestedViews[i].view || !nestedViews[i].proxy || nestedViews[i].proxy->widget()!=nestedViews[i].view) {
            if(nestedViews[i].view) {nestedViews[i].view->removeEventFilter(this);nestedViews[i].view->viewport()->removeEventFilter(this);}
            for(const auto &connection:nestedViews[i].connections)disconnect(connection);
            nestedViews.removeAt(i);
        }
        knownItems=scene->items().size();
        for(auto item:scene->items())if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
            if(!proxies.contains(proxy)) {
                proxies.append(proxy);
                connections.append(connect(proxy,&QGraphicsWidget::geometryChanged,this,[this]{requestLayout();}));
                connections.append(connect(proxy,&QGraphicsObject::visibleChanged,this,[this]{requestLayout();}));
                connections.append(connect(proxy,&QObject::destroyed,this,[this]{requestLayout();}));
            }
            if(auto nested=qobject_cast<QGraphicsView *>(proxy->widget()))watchNested(nested,proxy);
        }
    }
    void watchEditor(QWidget *editor) {
        for(int i=editors.size()-1;i>=0;--i)if(!editors[i])editors.removeAt(i);
        if(!editors.contains(editor)) {editors.append(editor);editor->installEventFilter(this);}
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(!view)return false;
        if(event->type()==QEvent::KeyPress || event->type()==QEvent::InputMethod)
            NativeCommandActivity::shared()->completed(QStringLiteral("native-timeline-input"));
        refresh();
        if(event->type()==QEvent::Paint && captureActive())return false;
        if(object==view->viewport() && event->type()==QEvent::Paint) {
            QList<QRectF> rects;
            for(const auto &rect:static_cast<QPaintEvent *>(event)->region())rects.append(view->mapToScene(rect).boundingRect());
            if(damaged)damaged(rects);
        } else if(event->type()==QEvent::Paint && !view->viewport()->testAttribute(Qt::WA_WState_InPaintEvent)) {
            // Child paints caused by the parent's paint are already covered by
            // that parent's damage; they are not new full-widget invalidations.
            for(const auto &entry:nestedViews)if(entry.view && object==entry.view->viewport()) {
                if(entry.view->scene()!=entry.scene)requestLayout();
                mappedDamage(entry.view,entry.proxy,static_cast<QPaintEvent *>(event)->region());
                break;
            }
        }
        if(object==view && event->type()==QEvent::Hide) {
            layout.stop();paint.stop();pendingPaint=QRegion();
            QTimer::singleShot(0,this,[this]{if(view && !view->isVisible() && suspended)suspended();});
        }
        switch(event->type()) {
        case QEvent::Show:case QEvent::LayoutRequest:case QEvent::ChildAdded:case QEvent::ChildPolished:
        case QEvent::Resize:case QEvent::StyleChange:case QEvent::FontChange:case QEvent::PaletteChange:
            requestLayout();break;
        default:break;
        }
        if(repaint && ((event->type()==QEvent::Paint && object!=view && object!=view->viewport()) || event->type()==QEvent::KeyPress ||
            event->type()==QEvent::InputMethod || event->type()==QEvent::FocusIn || event->type()==QEvent::FocusOut))repaint();
        return false;
    }
private:
    struct Nested {
        QPointer<QGraphicsView> view;QPointer<QGraphicsProxyWidget> proxy;QPointer<QGraphicsScene> scene;
        QList<QMetaObject::Connection> connections;
    };
    void clearNested() {
        for(const auto &entry:nestedViews) {
            if(entry.view){entry.view->removeEventFilter(this);entry.view->viewport()->removeEventFilter(this);}
            for(const auto &connection:entry.connections)disconnect(connection);
        }
        nestedViews.clear();
    }
    void mappedDamage(QGraphicsView *nested,QGraphicsProxyWidget *proxy,const QRegion &region) {
        if(!damaged || !nested || !proxy || proxy->widget()!=nested)return;
        QList<QRectF> rects;
        for(const auto &rect:region)rects.append(proxy->mapRectToScene(QRectF(rect).translated(nested->viewport()->pos())));
        if(rects.isEmpty())rects.append(proxy->sceneBoundingRect());
        damaged(rects);
    }
    void watchNested(QGraphicsView *nested,QGraphicsProxyWidget *proxy) {
        int index=0;while(index<nestedViews.size() && nestedViews[index].view!=nested)++index;
        if(index==nestedViews.size())nestedViews.append({nested,proxy,nullptr,{}});
        auto &entry=nestedViews[index];
        if(entry.scene==nested->scene() && !entry.connections.isEmpty())return;
        for(const auto &connection:entry.connections)disconnect(connection);
        entry.connections.clear();entry.scene=nested->scene();entry.proxy=proxy;
        nested->installEventFilter(this);nested->viewport()->installEventFilter(this);
        const bool header=nested->objectName().endsWith("TrackControlsView");
        QPointer<QGraphicsView> alive=nested;QPointer<QGraphicsProxyWidget> surface=proxy;
        QPointer<QGraphicsScene> original=nested->scene();
        if(original)entry.connections.append(connect(original,&QGraphicsScene::changed,this,
            [this,alive,surface,original,header](const QList<QRectF> &areas) {
                if(!alive || !surface || surface->widget()!=alive || alive->scene()!=original)return;
                if(header)requestLayout();
                QRegion region;
                for(const auto &area:areas)region+=alive->viewportTransform().mapRect(area).toAlignedRect();
                mappedDamage(alive,surface,region);
            }));
        auto scrolled=[this,alive,surface,header] {
            if(header || (headerMovesWithTime && headerMovesWithTime()))requestLayout();
            mappedDamage(alive,surface,{});
        };
        entry.connections.append(connect(nested->horizontalScrollBar(),&QScrollBar::valueChanged,this,scrolled));
        entry.connections.append(connect(nested->verticalScrollBar(),&QScrollBar::valueChanged,this,scrolled));
    }
    bool captureActive() const {return capturing && capturing();}
    void requestLayout() {
        if(stopped || !view || !view->isVisible())return;
        if(captureActive()){deferredLayout=true;return;}
        if(!layout.isActive())layout.start(0);
    }
    QPointer<QGraphicsView> view;QPointer<QGraphicsScene> scene;
    quint64 commandSubscription=0;
    QTimer layout,paint;QRegion pendingPaint;int knownItems=-1;bool stopped=false,deferredLayout=false;
    QList<QPointer<QGraphicsProxyWidget>> proxies;QVector<Nested> nestedViews;
    QList<QPointer<QWidget>> editors;QList<QMetaObject::Connection> connections;
};
}
