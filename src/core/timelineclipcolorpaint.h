// SPDX-License-Identifier: MIT
#pragma once
#include "timelineclipcolorbindings.h"
#include "clipfillpaintdevice.h"
#include "diagnostics.h"
namespace Unbound {
// Scope one native capture's rectangular fills using the existing root bindings.
// The renderer retains pixels/damage; this adapter owns only report deduplication.
class TimelineClipColorPaint {
public:
    void render(QGraphicsView *view,QPainter *target,const std::function<void(QPainter *)> &native) {
        auto bindings=TimelineClipColorBindings::find(view);
        const auto regions=bindings?bindings->regions():QVector<ClipFillRegion>();
        bool invertible=false;const auto inverse=target->deviceTransform().inverted(&invertible);
        if(regions.isEmpty() || !invertible){native(target);return;}
        const auto result=ClipFillPaintDevice::mapRectangularFills(target,
            [regions,inverse](const QRectF &r,QColor source,const QTransform &device){
                return clipRegionReplacement((device*inverse).mapRect(r),source,regions);
            },native);
        if(reported!=bindings || revision!=bindings->revision()) {
            reported=bindings;revision=bindings->revision();
            logEvent("clip-color.viewport.paint",{{"changed_fill_commands",result.changed},{"rectangles",result.trace.rects},
                {"pixmaps",result.trace.pixmaps},{"regions",regions.size()},{"project_writes",false}});
        }
    }
private:QPointer<TimelineClipColorBindings> reported;quint64 revision=0;
};
}
