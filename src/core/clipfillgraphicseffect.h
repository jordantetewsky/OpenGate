// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsEffect>
#include <QPointer>
#include "clipfillpaintdevice.h"
#include "diagnostics.h"
namespace Unbound {
// Qt graphics-item paint adapter only. Selection and effect leases live elsewhere.
class ClipFillGraphicsEffect : public QGraphicsEffect {
public:
    explicit ClipFillGraphicsEffect(QObject *owner):leaseOwner(owner){}
    QObject *owner() const {return leaseOwner;}
    void setColor(QColor value){color=value;reported=false;update();}
protected:
    void draw(QPainter *target) override {
        const auto result=ClipFillPaintDevice::render(target,color,[this](QPainter *p){drawSource(p);});
        if(!reported){reported=true;logEvent("clip-color.preview.paint",{
            {"source_fill",result.source.color.isValid()?result.source.color.name():QString()},
            {"target",color.name()},{"candidate_area",result.source.area},
            {"changed_fill_commands",result.changed},{"temporary",true}});}
    }
private:
    QPointer<QObject> leaseOwner;QColor color;bool reported=false;
};
}
