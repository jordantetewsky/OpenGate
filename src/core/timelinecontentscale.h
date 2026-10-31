// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QPointer>
#include <QtMath>
#include <QTimer>
#include <QScopedValueRollback>
#include "diagnostics.h"
namespace Unbound {
// Opt-in transform experiment for ordinary Qt scenes. Resolve's native Timeline
// paint engine does not scale glyphs/borders coherently through this path.
class TimelineContentScale : public QObject {
public:
    TimelineContentScale() {
        pending.setSingleShot(true);
        connect(&pending,&QTimer::timeout,this,[this]{if(root&&!applying)apply(root,amount);});
    }
    ~TimelineContentScale() { restore(); }
    static bool enabled() { return qEnvironmentVariableIntValue("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT")==1; }
    static QList<QGraphicsView *> surfaces(QWidget *root) {
        QList<QGraphicsView *> result;
        if(root)for(auto view:root->findChildren<QGraphicsView *>("m_TiTimelineView")) {
            if(!view->scene())continue;
            if(!view->viewport())return {};
            // Observing a QGraphicsView base class is not a renderer capability.
            // BMD's paint engine maps text positions separately from glyph
            // drawing, so even an explicit experiment flag cannot opt it in.
            if(view->viewport()->inherits("BmdDavUI::UiGraphicsViewportWidget") ||
               view->viewport()->testAttribute(Qt::WA_NativeWindow) ||
               view->viewport()->testAttribute(Qt::WA_PaintOnScreen))return {};
            result.append(view);
        }
        return result;
    }
    static bool supported(QWidget *root) { return enabled() && !surfaces(root).isEmpty(); }
    bool apply(QWidget *surface,double factor) {
        if(!enabled() || !qIsFinite(factor) || factor<0.7 || factor>3.0)return false;
        if(root!=surface){restore();root=surface;}
        amount=factor;watch(surface);
        const auto candidates=surfaces(surface);if(candidates.isEmpty())return false;
        QScopedValueRollback<bool> guard(applying,true);
        for(auto view:candidates) {
            bool known=false;for(const auto &entry:views)if(entry.view==view){known=true;break;}
            if(!known)views.append({view,view->transform(),view->horizontalScrollBarPolicy(),view->verticalScrollBarPolicy()});
        }
        for(const auto &entry:views)if(entry.view) {
            const auto center=entry.view->mapToScene(entry.view->viewport()->rect().center());
            entry.view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            entry.view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            entry.view->setTransform(entry.baseline*QTransform::fromScale(factor,factor));
            entry.view->centerOn(center);
        }
        logEvent("timeline.content.scale.experiment",{{"factor",factor},{"views",views.size()},
            {"native_live_verified",false},{"toolbar_scaled",false}});
        return true;
    }
    void restore() {
        pending.stop();QScopedValueRollback<bool> guard(applying,true);
        for(auto widget:watched)if(widget)widget->removeEventFilter(this);
        watched.clear();
        for(const auto &entry:views)if(entry.view) {
            const auto center=entry.view->mapToScene(entry.view->viewport()->rect().center());
            entry.view->setTransform(entry.baseline);
            entry.view->setHorizontalScrollBarPolicy(entry.horizontal);entry.view->setVerticalScrollBarPolicy(entry.vertical);
            entry.view->centerOn(center);
        }
        views.clear();root=nullptr;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(applying || !root)return false;
        if(event->type()==QEvent::ChildAdded || event->type()==QEvent::ChildPolished || event->type()==QEvent::Show)pending.start(0);
        if(event->type()==QEvent::Resize)if(auto view=qobject_cast<QGraphicsView *>(object))
            for(const auto &entry:views)if(entry.view==view && view->transform()!=entry.baseline*QTransform::fromScale(amount,amount))pending.start(0);
        return false;
    }
private:
    void watch(QWidget *surface) {
        for(int i=watched.size()-1;i>=0;--i)if(!watched[i])watched.removeAt(i);
        auto widgets=surface?surface->findChildren<QWidget *>():QList<QWidget *>{};if(surface)widgets.prepend(surface);
        for(auto widget:widgets) {
            const auto name=widget->objectName();
            if(widget!=surface && name!="m_TiTimelineView" && name!="UiTiLayoutContainer" && name!="timelineViewsSplitter" && name!="frameEditTimelinesContainer")continue;
            if(!watched.contains(widget)){watched.append(widget);widget->installEventFilter(this);}
        }
    }
    QTimer pending;QList<QPointer<QWidget>> watched;bool applying=false;double amount=1.;
    struct View { QPointer<QGraphicsView> view; QTransform baseline; Qt::ScrollBarPolicy horizontal,vertical; };
    QPointer<QWidget> root;QList<View> views;
};
}
