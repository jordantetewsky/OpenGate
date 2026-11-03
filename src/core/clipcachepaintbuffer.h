// SPDX-License-Identifier: MIT
#pragma once
#include <QPixmap>
#include <memory>
#include "clipfillpaintdevice.h"

namespace Unbound {
// Match Qt device-coordinate cache raster format, DPR and rounded pixel origin.
// Own backing pixels only; item policies and fill replacement live elsewhere.
class ClipCachePaintBuffer {
public:
    QPainter *begin(QPainter *target,QRectF deviceBounds) {
        bounds=deviceBounds.toRect().adjusted(-1,-1,1,1);
        const int ratio=qMax(1,target->device()->devicePixelRatio());
        const auto size=bounds.size()*ratio;
        if(size.isEmpty() || size.width()>16384 || size.height()>16384 || qint64(size.width())*size.height()>16*1024*1024)return nullptr;
        if(pixels.size()!=size || pixels.devicePixelRatio()!=ratio){pixels=QPixmap(size);pixels.setDevicePixelRatio(ratio);}
        pixels.fill(Qt::transparent);paint=std::make_unique<QPainter>(&pixels);
        ClipFillPaintDevice::State state(*target);
        state.transform*=QTransform::fromTranslate(-bounds.x(),-bounds.y());state.apply(*paint);
        return paint.get();
    }
    void present(QPainter *target) {
        paint.reset();target->save();target->setWorldTransform(QTransform());
        target->drawPixmap(bounds.topLeft(),pixels);target->restore();
    }
private:
    QPixmap pixels;QRect bounds;std::unique_ptr<QPainter> paint;
};
}
