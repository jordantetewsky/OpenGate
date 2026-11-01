// SPDX-License-Identifier: MIT
#pragma once
#include "panelmetricscale.h"
#include "timelinecontentscale.h"
#include "mediapoolcontentscale.h"
namespace Unbound {
// One panel's retained scale value and reversible metric application.
// Session transfer calls suspend/apply; workspace persistence reads value().
class PanelContentScale {
public:
    PanelContentScale() {
        mediaPool.changed=[this]{if(changed)changed();};
        timeline.changed=[this]{if(changed)changed();};
    }
    std::function<void()> changed;
    QPoint scrollOffset() const { return mediaPool.offset(); }
    void restoreScrollOffset(QPoint value) { mediaPool.restoreOffset(value); }
    QJsonObject snapshot() const {
        const auto offset=scrollOffset();
        auto saved=timeline.snapshot();
        saved["interface_scale"]=factor;
        saved["content_scroll"]=QJsonArray{offset.x(),offset.y()};
        saved["content_canvas_width"]=mediaPool.canvasWidth();
        return saved;
    }
    void restoreState(const QJsonObject &saved) {
        timeline.restoreState(saved);
        mediaPool.restoreCanvasWidth(saved["content_canvas_width"].toInt());
        const auto offset=saved["content_scroll"].toArray();restoreScrollOffset({offset.at(0).toInt(),offset.at(1).toInt()});
        setValue(saved["interface_scale"].toDouble(1.0));
    }
    double value() const { return factor; }
    bool setValue(double next) {
        if (!qIsFinite(next) || next<0.7 || next>3.0) return false;
        factor=next;return true;
    }
    bool suspend() {
        metrics.restore();
        const bool timelineRestored=timeline.restore();
        const bool mediaPoolRestored=mediaPool.restore();
        return timelineRestored&&mediaPoolRestored;
    }
    static bool supported(QWidget *surface) {
        if(!surface)return false;
        if(surface->objectName()=="frameEditPageMediaPool")return MediaPoolContentScale::supported(surface);
        return !isTimeline(surface) || TimelineContentScale::supported(surface);
    }
    bool apply(QWidget *surface) {
        if (!surface) return suspend();
        if(isTimeline(surface)) {
            metrics.restore();
            if(!mediaPool.restore())return false;
            return timeline.apply(surface,factor);
        }
        if(qFuzzyCompare(factor,1.0))return suspend();
        if(surface->objectName()=="frameEditPageMediaPool") {
            metrics.restore();
            if(!timeline.restore())return false;
            return mediaPool.apply(surface,factor);
        }
        if(!mediaPool.restore())return false;
        if(!timeline.restore())return false;
        return metrics.attach(surface,factor);
    }
private:
    static bool isTimeline(QWidget *surface) { return surface->objectName()=="frameConformTimelines"; }
    TimelineContentScale timeline;
    MediaPoolContentScale mediaPool;
    PanelMetricScale metrics;
    double factor=1.0;
};
}
