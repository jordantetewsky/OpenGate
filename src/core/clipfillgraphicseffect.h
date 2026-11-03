// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsEffect>
#include <QPointer>
#include <QTimer>
#include "clipfillpaintdevice.h"
#include "clipdevicecachelease.h"
#include "clipcachepaintbuffer.h"
#include "diagnostics.h"
namespace Unbound {
// Qt graphics-item paint adapter only. Selection and effect leases live elsewhere.
class ClipFillGraphicsEffect : public QGraphicsEffect {
public:
    ClipFillGraphicsEffect(QObject *owner,ClipDeviceCacheLease *cache):leaseOwner(owner),cache(cache){}
    QObject *owner() const {return leaseOwner;}
    std::function<void()> unsupported;
    void setColor(QColor value){color=value;reported=false;update();}
protected:
    void draw(QPainter *target) override {
        if(cache)cache->refresh();
        // Native item caches composite alpha per item. A common backing cannot
        // reproduce that composition for translucent items without changing ink.
        const bool opacityUnsupported=(cache && cache->unsupportedOpacityItems()>0) || !qFuzzyCompare(target->opacity(),qreal(1));
        const bool buffered=cache && cache->leased()>0;
        auto output=opacityUnsupported?nullptr:(buffered?backing.begin(target,sourceBoundingRect(Qt::DeviceCoordinates)):target);
        ClipFillPaintDevice::Result result;
        if(output){result=ClipFillPaintDevice::render(output,color,[this](QPainter *p){drawSource(p);});if(buffered)backing.present(target);}
        else {if(cache)cache->release();drawSource(target);}
        if(!reported){reported=true;QJsonArray paths;for(int count:result.trace.pathElements)paths.append(count);
            logEvent("clip-color.preview.paint",{
            {"source_fill",result.source.color.isValid()?result.source.color.name():QString()},
            {"target",color.name()},{"candidate_area",result.source.area},
            {"changed_fill_commands",result.changed},{"temporary",true},
            {"paths",result.trace.paths},{"rounded_paths",result.trace.rounded},{"path_elements",paths},
            {"rectangles",result.trace.rects},{"images",result.trace.images},{"pixmaps",result.trace.pixmaps},
            {"gradient_brushes",result.trace.gradients},{"texture_brushes",result.trace.textures},
            {"qt_device_cache_leases",cache?cache->leased():0},
            {"cache_backing",buffered && output!=nullptr},
            {"unsupported_item_caches",cache?cache->unsupportedItemCaches():0},
            {"unsupported_plain_caches",cache?cache->unsupportedPlainCaches():0},
            {"unsupported_opacity_items",cache?cache->unsupportedOpacityItems():0}});
            if(!result.changed)QTimer::singleShot(0,this,[this]{const auto callback=unsupported;if(callback)callback();});
        }
    }
private:
    QPointer<QObject> leaseOwner;QPointer<ClipDeviceCacheLease> cache;ClipCachePaintBuffer backing;QColor color;bool reported=false;
};
}
