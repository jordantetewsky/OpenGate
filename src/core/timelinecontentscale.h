// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QPointer>
#include <QtMath>
#include "diagnostics.h"
namespace Unbound {
// Experimental native scene transform. Qt owns vector rendering and inverse input
// mapping. No screenshot, proxy replacement, toolbar or native timeline zoom change.
class TimelineContentScale {
public:
    ~TimelineContentScale() { restore(); }
    static bool enabled() { return qEnvironmentVariableIntValue("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT")==1; }
    static QList<QGraphicsView *> surfaces(QWidget *root) {
        QList<QGraphicsView *> result;
        if(root)for(auto view:root->findChildren<QGraphicsView *>("m_TiTimelineView")) {
            if(!view->scene() || !view->viewport() || view->viewport()->testAttribute(Qt::WA_NativeWindow) ||
               view->viewport()->testAttribute(Qt::WA_PaintOnScreen))return {};
            result.append(view);
        }
        return result;
    }
    static bool supported(QWidget *root) { return enabled() && !surfaces(root).isEmpty(); }
    bool apply(QWidget *surface,double factor) {
        if(!enabled() || !qIsFinite(factor) || factor<0.7 || factor>3.0)return false;
        const auto candidates=surfaces(surface);if(candidates.isEmpty())return false;
        if(root!=surface){restore();root=surface;}
        for(auto view:candidates) {
            bool known=false;for(const auto &entry:views)if(entry.view==view){known=true;break;}
            if(!known)views.append({view,view->transform()});
        }
        for(const auto &entry:views)if(entry.view) {
            const auto center=entry.view->mapToScene(entry.view->viewport()->rect().center());
            entry.view->setTransform(entry.baseline*QTransform::fromScale(factor,factor));
            entry.view->centerOn(center);
        }
        logEvent("timeline.content.scale.experiment",{{"factor",factor},{"views",views.size()},
            {"native_live_verified",false},{"toolbar_scaled",false}});
        return true;
    }
    void restore() {
        for(const auto &entry:views)if(entry.view) {
            const auto center=entry.view->mapToScene(entry.view->viewport()->rect().center());
            entry.view->setTransform(entry.baseline);entry.view->centerOn(center);
        }
        views.clear();root=nullptr;
    }
private:
    struct View { QPointer<QGraphicsView> view; QTransform baseline; };
    QPointer<QWidget> root;QList<View> views;
};
}
