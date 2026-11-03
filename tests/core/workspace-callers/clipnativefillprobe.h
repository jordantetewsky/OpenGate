// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsEffect>
#include "../../../src/core/timelineclipcolorbindings.h"
#include "../../../src/core/clipfillpaintdevice.h"

namespace OpenGateTest {
// Development-only interception around a complete native view, including its
// drawBackground pass. Does not replace the installed renderer or own bindings.
class ClipNativeFillProbe : public QGraphicsEffect {
public:
    using Report=std::function<void(const Unbound::ClipFillPaintDevice::Result &)>;
    using Inspect=std::function<void(const char *,const QRectF &,QColor,QColor)>;
    ClipNativeFillProbe(QGraphicsView *root,QGraphicsProxyWidget *proxy,Report report={},Inspect inspect={})
        :root(root),proxy(proxy),report(std::move(report)),inspect(std::move(inspect)){}
protected:
    void draw(QPainter *painter) override {
        auto bindings=Unbound::TimelineClipColorBindings::find(root);
        if(!bindings || !proxy){drawSource(painter);return;}
        const auto regions=bindings->regions();
        bool valid=false;const auto inverse=painter->deviceTransform().inverted(&valid);
        if(!valid || regions.isEmpty()){drawSource(painter);return;}
        const auto toRoot=proxy->sceneTransform();
        const auto result=Unbound::ClipFillPaintDevice::mapRectangularFills(painter,
            [this,regions,inverse,toRoot](const QRectF &rect,QColor color,const QTransform &device){
                const auto mapped=(device*inverse*toRoot).mapRect(rect);
                const auto replacement=Unbound::clipRegionReplacement(mapped,color,regions);
                if(inspect && !reported)inspect("rectangle",mapped,color,replacement);
                return replacement;
            },[this](QPainter *target){drawSource(target);},
            [this,inverse,toRoot](const QRectF &rect,const QTransform &device,const char *kind){
                if(inspect && !reported)inspect(kind,(device*inverse*toRoot).mapRect(rect),{},{});
            });
        if(report && !reported){reported=true;report(result);}
    }
private:
    QPointer<QGraphicsView> root;QPointer<QGraphicsProxyWidget> proxy;Report report;Inspect inspect;bool reported=false;
};
}
