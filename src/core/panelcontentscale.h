// SPDX-License-Identifier: MIT
#pragma once
#include "panelmetricscale.h"
#include "timelinecontentscale.h"
namespace Unbound {
// One panel's retained scale value and reversible metric application.
// Session transfer calls suspend/apply; workspace persistence reads value().
class PanelContentScale {
public:
    double value() const { return factor; }
    bool setValue(double next) {
        if (!qIsFinite(next) || next<0.7 || next>3.0) return false;
        factor=next;return true;
    }
    void suspend() { metrics.restore(); timeline.restore(); }
    static bool supported(QWidget *surface) {
        return surface && (!isTimeline(surface) || TimelineContentScale::supported(surface));
    }
    bool apply(QWidget *surface) {
        if (!surface || qFuzzyCompare(factor,1.0)) { suspend();return true; }
        if(isTimeline(surface)) {
            metrics.restore();
            if(timeline.apply(surface,factor))return true;
            timeline.restore();
            logEvent("timeline.content.scale.unavailable",{{"factor",factor},{"toolbar_scaled",false},
                {"reason","native-renderer-verification-required"}});
            return false;
        }
        timeline.restore();return metrics.attach(surface,factor);
    }
private:
    static bool isTimeline(QWidget *surface) { return surface->objectName()=="frameConformTimelines"; }
    TimelineContentScale timeline;
    PanelMetricScale metrics;
    double factor=1.0;
};
}
