// SPDX-License-Identifier: MIT
#pragma once
#include "timelineclipcolorbindings.h"
#include "clipfillpaintdevice.h"
#include "diagnostics.h"
#include "clipskinpaint.h"
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
        skins.begin(regions);
        const auto result=ClipFillPaintDevice::mapRectangularFills(target,
            [regions,inverse](const QRectF &r,QColor source,const QTransform &device){
                return clipRegionReplacement((device*inverse).mapRect(r),source,regions);
            },native,{},[this,inverse](const QRectF &r,const QPixmap &pixmap,const QRectF &source,const QTransform &device){
                return skins.map((device*inverse).mapRect(r),pixmap,source);
            });
        if(skins.learned()){QPointer<QGraphicsView> alive=view;QTimer::singleShot(0,view,[alive]{if(alive){alive->scene()->invalidate();alive->viewport()->update();}});}
        const bool first=reported!=bindings || revision!=bindings->revision();
        if(first)reportedRaster=false;
        if(first || (!reportedRaster && result.changedPixmaps>0)) {
            reported=bindings;revision=bindings->revision();
            reportedRaster=result.changedPixmaps>0;
            logEvent("clip-color.viewport.paint",{{"changed_fill_commands",result.changed},{"rectangles",result.trace.rects},
                {"pixmaps",result.trace.pixmaps},{"changed_skin_pixmaps",result.changedPixmaps},{"regions",regions.size()},{"project_writes",false}});
        }
    }
private:QPointer<TimelineClipColorBindings> reported;quint64 revision=0;bool reportedRaster=false;ClipSkinPaint skins;
};
}
