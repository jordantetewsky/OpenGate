// SPDX-License-Identifier: MIT
#pragma once
#include "panelmetricscale.h"
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
    void suspend() { metrics.restore(); }
    void apply(QWidget *surface) {
        if (!surface || qFuzzyCompare(factor,1.0)) { suspend();return; }
        metrics.attach(surface,factor);
    }
private:
    PanelMetricScale metrics;
    double factor=1.0;
};
}
