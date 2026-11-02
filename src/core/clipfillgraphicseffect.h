// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsEffect>
#include <QPointer>
#include <QTimer>
#include "clipfillpaintdevice.h"
#include "diagnostics.h"
namespace Unbound {
// Qt graphics-item paint adapter only. Selection and effect leases live elsewhere.
class ClipFillGraphicsEffect : public QGraphicsEffect {
public:
    explicit ClipFillGraphicsEffect(QObject *owner):leaseOwner(owner){}
    QObject *owner() const {return leaseOwner;}
    std::function<void()> unsupported;
    void setColor(QColor value){color=value;reported=false;update();}
protected:
    void draw(QPainter *target) override {
        const auto result=ClipFillPaintDevice::render(target,color,[this](QPainter *p){drawSource(p);});
        if(!reported){reported=true;QJsonArray paths;for(int count:result.trace.pathElements)paths.append(count);
            logEvent("clip-color.preview.paint",{
            {"source_fill",result.source.color.isValid()?result.source.color.name():QString()},
            {"target",color.name()},{"candidate_area",result.source.area},
            {"changed_fill_commands",result.changed},{"temporary",true},
            {"paths",result.trace.paths},{"rounded_paths",result.trace.rounded},{"path_elements",paths},
            {"rectangles",result.trace.rects},{"images",result.trace.images},{"pixmaps",result.trace.pixmaps},
            {"gradient_brushes",result.trace.gradients},{"texture_brushes",result.trace.textures}});
            if(!result.changed)QTimer::singleShot(0,this,[this]{const auto callback=unsupported;if(callback)callback();});
        }
    }
private:
    QPointer<QObject> leaseOwner;QColor color;bool reported=false;
};
}
