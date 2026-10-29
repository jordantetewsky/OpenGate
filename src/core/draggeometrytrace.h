// SPDX-License-Identifier: MIT
#pragma once
#include "diagnostics.h"
#include "../platform/workspacewindowintegration.h"
#include <QWidget>
#include <QWindow>

namespace Unbound {
// Read-only coordinate evidence. Bounded to one detailed record per drag, not a
// synchronous log on every mouse move. Never compensates or changes geometry.
class DragGeometryTrace {
public:
    void begin() { samples=mismatches=maximum=0; }
    void sample(QWidget *host,const QPoint &pointer,const QRect &desired,const char *stage)
    {
        ++samples;
        const auto actual=host->geometry();
        const auto delta=actual.topLeft()-desired.topLeft();
        if (delta.isNull()) return;
        maximum=qMax(maximum,delta.manhattanLength()); ++mismatches;
        if (mismatches!=1) return;
        auto data=nativeWindowGeometryState(host);
        data["window"]=host->objectName(); data["stage"]=stage;
        data["pointer_x"]=pointer.x(); data["pointer_y"]=pointer.y();
        data["desired_x"]=desired.x(); data["desired_y"]=desired.y();
        data["actual_x"]=actual.x(); data["actual_y"]=actual.y();
        if (auto window=host->windowHandle()) {
            data["qwindow_x"]=window->x(); data["qwindow_y"]=window->y();
        }
        logEvent("drag.geometry.mismatch",data);
    }
    void report(const QString &window) const
    {
        logEvent("drag.geometry.summary",{{"window",window},{"samples",samples},{"mismatches",mismatches},{"maximum_delta",maximum}});
    }
private:
    int samples=0,mismatches=0,maximum=0;
};
}
