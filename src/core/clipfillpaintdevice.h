// SPDX-License-Identifier: MIT
#pragma once
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <cmath>
#include <functional>

namespace Unbound {
// Forward native paint commands; optional hooks replace explicitly identified
// backgrounds/labels. This device knows no clip, preset, input or project model.
class ClipFillPaintDevice : public QPaintDevice {
public:
    using FillMapper=std::function<QColor(const QRectF &,QColor,const QTransform &)>;
    using RasterObserver=std::function<void(const QRectF &,const QTransform &,const char *)>;
    using PixmapMapper=std::function<QPixmap(const QRectF &,const QPixmap &,const QRectF &,const QTransform &)>;
    struct PaintHooks {
        std::function<QBrush(const QRectF &,QColor,const QTransform &)> brush;
        std::function<void(QPainter *,const QRectF &,const QTransform &)> afterBody;
        std::function<bool(QPainter *,const QRectF &,const QPixmap &,const QRectF &,const QTransform &)> pixmap;
        std::function<bool(QPainter *,const QPointF &,const QTextItem &,const QTransform &)> text;
    };
    struct Fill {QColor color;double area=0.;};
    struct Trace {int paths=0,rounded=0,rects=0,images=0,pixmaps=0,gradients=0,textures=0;QVector<int> pathElements;};
    ClipFillPaintDevice(QPainter *output,QColor source={},QColor replacement={},QPaintDevice *reference=nullptr,FillMapper mapper={},RasterObserver rasterObserver={},PixmapMapper pixmapMapper={},PaintHooks hooks={})
        : engine(output,source,replacement,std::move(mapper),std::move(rasterObserver),std::move(pixmapMapper),std::move(hooks)),reference(reference?reference:output?output->device():nullptr) {}
    QPaintEngine *paintEngine() const override {return const_cast<Engine *>(&engine);}
    Fill largestFill() const {return engine.largest;}
    int changedFillCommands() const {return engine.changed;}
    struct State {
        QTransform transform;QPen pen;QBrush brush,background;QFont font;QPointF origin;
        QPainter::RenderHints hints;QPainter::CompositionMode composition;
        Qt::BGMode backgroundMode;double opacity;bool clipped;QPainterPath clip;
        explicit State(const QPainter &p):transform(p.worldTransform()),pen(p.pen()),brush(p.brush()),
            background(p.background()),font(p.font()),origin(p.brushOrigin()),hints(p.renderHints()),
            composition(p.compositionMode()),backgroundMode(p.backgroundMode()),opacity(p.opacity()),
            clipped(p.hasClipping()),clip(clipped?p.clipPath():QPainterPath()) {}
        void apply(QPainter &p) const {
            p.setWorldTransform(transform);p.setPen(pen);p.setBrush(brush);p.setFont(font);p.setBrushOrigin(origin);
            p.setBackground(background);p.setBackgroundMode(backgroundMode);p.setRenderHints(hints);
            p.setOpacity(opacity);p.setCompositionMode(composition);if(clipped)p.setClipPath(clip);
        }
    };
    struct Result {Fill source;int changed=0;Trace trace;int changedPixmaps=0;};
    static Result mapRectangularFills(QPainter *target,FillMapper mapper,const std::function<void(QPainter *)> &source,RasterObserver observer={},PixmapMapper pixmaps={},PaintHooks hooks={}) {
        const State state(*target);ClipFillPaintDevice device(target,{},{},nullptr,std::move(mapper),std::move(observer),std::move(pixmaps),std::move(hooks));device.nativeMetrics=true;
        {QPainter output(&device);state.apply(output);source(&output);}
        return {device.largestFill(),device.changedFillCommands(),device.engine.trace,device.engine.changedPixmaps};
    }
    static Result render(QPainter *target,QColor color,const std::function<void(QPainter *)> &source,
                         const std::function<void(QPainter *)> &inspectSource={}) {
        const State state(*target);ClipFillPaintDevice inspect(nullptr,{},{},target->device());
        {QPainter scan(&inspect);state.apply(scan);if(inspectSource)inspectSource(&scan);else source(&scan);}
        const auto fill=inspect.largestFill();ClipFillPaintDevice device(target,fill.color,color);
        {QPainter output(&device);state.apply(output);source(&output);}
        return {fill,device.changedFillCommands(),inspect.engine.trace};
    }
protected:
    int metric(PaintDeviceMetric key) const override {
        switch(key) {
        case PdmWidth:return nativeMetrics && reference?reference->width():16384;
        case PdmHeight:return nativeMetrics && reference?reference->height():16384;
        case PdmWidthMM:case PdmHeightMM:return 4335;
        case PdmDpiX:return reference?reference->logicalDpiX():96;
        case PdmDpiY:return reference?reference->logicalDpiY():96;
        case PdmPhysicalDpiX:return reference?reference->physicalDpiX():96;
        case PdmPhysicalDpiY:return reference?reference->physicalDpiY():96;
        case PdmDepth:return 32;
        case PdmNumColors:return 0;
        case PdmDevicePixelRatio:return nativeMetrics && reference?reference->devicePixelRatio():1;
        case PdmDevicePixelRatioScaled:return int(devicePixelRatioFScale()*(nativeMetrics && reference?reference->devicePixelRatioF():1.));
        }
        return 0;
    }
private:
    class Engine : public QPaintEngine {
    public:
        Engine(QPainter *output,QColor source,QColor replacement,FillMapper mapper,RasterObserver observer,PixmapMapper pixmaps,PaintHooks hooks)
            :QPaintEngine(AllFeatures),out(output),source(source),replacement(replacement),mapper(std::move(mapper)),rasterObserver(std::move(observer)),pixmapMapper(std::move(pixmaps)),hooks(std::move(hooks)) {}
        Fill largest;int changed=0,changedPixmaps=0;Trace trace;
        bool begin(QPaintDevice *) override {
            if(out){if(mapper)replayBasis=out->deviceTransform().inverted()*out->worldTransform();out->save();}
            return true;
        }
        bool end() override {if(out)out->restore();return true;}
        Type type() const override {return User;}
        void updateState(const QPaintEngineState &s) override {
            brush=s.brush();
            if(s.state().testFlag(DirtyBrush)){trace.gradients+=brush.gradient()!=nullptr;trace.textures+=brush.style()==Qt::TexturePattern;}
            if(!out)return;
            // Rebuild from the target's saved state so nested clipping and saves
            // cannot leak into neighboring clips or remove the capture's clip.
            out->restore();out->save();
            out->setWorldTransform(mapper?s.transform()*replayBasis:s.transform());
            out->setPen(s.pen());out->setBrush(brush);out->setBrushOrigin(s.brushOrigin());
            out->setFont(s.font());out->setBackground(s.backgroundBrush());
            out->setBackgroundMode(s.backgroundMode());out->setOpacity(s.opacity());
            out->setCompositionMode(s.compositionMode());
            out->setRenderHints(out->renderHints(),false);out->setRenderHints(s.renderHints());
            if(painter()->hasClipping())out->setClipPath(painter()->clipPath(),Qt::IntersectClip);
        }
        void drawPath(const QPainterPath &path) override {
            ++trace.paths;if(trace.pathElements.size()<8)trace.pathElements.append(path.elementCount());
            const bool rounded=roundedRectangle(path);trace.rounded+=rounded;
            const bool rectangular=rectangle(path)||rounded;
            if(rectangular)consider(path.boundingRect());
            if(out){out->setBrush(mapped(rectangular,path.boundingRect()));out->drawPath(path);if(styled){out->save();out->setClipPath(path,Qt::IntersectClip);after(path.boundingRect());out->restore();}out->setBrush(brush);}
        }
        void drawRects(const QRectF *r,int count) override {
            trace.rects+=count;
            for(int i=0;i<count;++i)consider(r[i]);
            if(out){
                if(mapper)for(int i=0;i<count;++i){out->setBrush(mapped(true,r[i]));out->drawRects(r+i,1);after(r[i]);}
                else {out->setBrush(mapped(true,{}));out->drawRects(r,count);}
                out->setBrush(brush);
            }
        }
        void drawRects(const QRect *r,int n) override {for(int i=0;i<n;++i){QRectF v(r[i]);drawRects(&v,1);}}
        void drawEllipse(const QRectF &r) override {if(out)out->drawEllipse(r);}
        void drawEllipse(const QRect &r) override {drawEllipse(QRectF(r));}
        void drawLines(const QLineF *r,int n) override {if(out)out->drawLines(r,n);}
        void drawLines(const QLine *r,int n) override {if(out)out->drawLines(r,n);}
        void drawPoints(const QPointF *p,int n) override {if(out)out->drawPoints(p,n);}
        void drawPoints(const QPoint *p,int n) override {if(out)out->drawPoints(p,n);}
        void drawPolygon(const QPointF *p,int n,PolygonDrawMode mode) override {
            QPainterPath path;
            if(n==4 && mode!=PolylineMode){path.moveTo(p[0]);for(int i=1;i<n;++i)path.lineTo(p[i]);path.closeSubpath();}
            const bool rectangular=rectangle(path);
            if(rectangular)consider(path.boundingRect());
            if(!out)return;
            out->setBrush(mapped(rectangular,path.boundingRect()));
            if(mode==PolylineMode)out->drawPolyline(p,n);
            else if(mode==ConvexMode)out->drawConvexPolygon(p,n);
            else out->drawPolygon(p,n,mode==WindingMode?Qt::WindingFill:Qt::OddEvenFill);
            after(path.boundingRect());
            out->setBrush(brush);
        }
        void drawPolygon(const QPoint *p,int n,PolygonDrawMode mode) override {
            QVector<QPointF> points;points.reserve(n);for(int i=0;i<n;++i)points.append(p[i]);
            drawPolygon(points.constData(),n,mode);
        }
        void drawPixmap(const QRectF &r,const QPixmap &p,const QRectF &s) override {
            ++trace.pixmaps;observeRaster(r,"pixmap");if(!out)return;
            if(hooks.pixmap&&hooks.pixmap(out,r,p,s,painter()->deviceTransform())){++changedPixmaps;return;}
            const auto replacement=pixmapMapper?pixmapMapper(r,p,s,painter()->deviceTransform()):QPixmap();
            if(!replacement.isNull()){++changedPixmaps;out->drawPixmap(r,replacement,s);}else out->drawPixmap(r,p,s);
        }
        void drawTiledPixmap(const QRectF &r,const QPixmap &p,const QPointF &s) override {++trace.pixmaps;observeRaster(r,"tiled-pixmap");if(out)out->drawTiledPixmap(r,p,s);}
        void drawImage(const QRectF &r,const QImage &p,const QRectF &s,Qt::ImageConversionFlags f) override {++trace.images;observeRaster(r,"image");if(out)out->drawImage(r,p,s,f);}
        void drawTextItem(const QPointF &p,const QTextItem &t) override {if(out&&!(hooks.text&&hooks.text(out,p,t,painter()->deviceTransform())))out->drawTextItem(p,t);}
    private:
        QPainter *out;QColor source,replacement;QBrush brush;FillMapper mapper;RasterObserver rasterObserver;PixmapMapper pixmapMapper;QTransform replayBasis;
        PaintHooks hooks;bool styled=false;
        void after(const QRectF &rect){if(styled&&hooks.afterBody){out->save();out->setClipRect(rect.adjusted(.5,.5,-.5,-.5),Qt::IntersectClip);hooks.afterBody(out,rect,painter()->deviceTransform());out->restore();}}
        void observeRaster(const QRectF &rect,const char *kind) {if(rasterObserver)rasterObserver(rect,painter()->deviceTransform(),kind);}
        static bool roundedRectangle(const QPainterPath &path) {
            if(path.elementCount()!=17)return false;
            const auto bounds=path.boundingRect();const auto start=path.elementAt(0),end=path.elementAt(3);
            const double rx=end.x-bounds.left(),ry=start.y-bounds.top();
            if(rx<=0 || ry<=0 || rx>bounds.width()/2 || ry>bounds.height()/2)return false;
            QPainterPath expected;expected.addRoundedRect(bounds,rx,ry);return path==expected;
        }
        static bool rectangle(const QPainterPath &path) {
            if(path.elementCount()!=5)return false;
            const auto bounds=path.boundingRect();
            if(bounds.isEmpty())return false;
            for(int i=0;i<5;++i){const auto p=path.elementAt(i);
                if(p.isCurveTo() || (p.x!=bounds.left() && p.x!=bounds.right()) ||
                   (p.y!=bounds.top() && p.y!=bounds.bottom()))return false;}
            for(int i=1;i<5;++i){const auto p=path.elementAt(i),previous=path.elementAt(i-1);
                if(p.x!=previous.x && p.y!=previous.y)return false;}
            for(int i=0;i<4;++i)for(int j=0;j<i;++j)
                if(path.elementAt(i).x==path.elementAt(j).x && path.elementAt(i).y==path.elementAt(j).y)return false;
            return path.elementAt(0).x==path.elementAt(4).x && path.elementAt(0).y==path.elementAt(4).y;
        }
        void consider(const QRectF &r) {
            const auto c=brush.color();const double area=std::abs(r.width()*r.height());
            if(brush.style()==Qt::SolidPattern && c.alpha()>0 && c.hsvSaturationF()>.25 &&
               c.valueF()>.2 && area>largest.area)largest={c,area};
        }
        QBrush mapped(bool rectangular,const QRectF &rect) {
            styled=false;
            if(rectangular&&hooks.brush&&brush.style()==Qt::SolidPattern){auto result=hooks.brush(rect,brush.color(),painter()->deviceTransform());if(result.style()!=Qt::NoBrush){styled=true;++changed;return result;}}
            if(rectangular && mapper && brush.style()==Qt::SolidPattern) {
                auto color=mapper(rect,brush.color(),painter()->deviceTransform());
                if(color.isValid()){++changed;color.setAlpha(brush.color().alpha());return QBrush(color);}
            }
            if(rectangular && source.isValid() && replacement.isValid() && brush.style()==Qt::SolidPattern &&
               brush.color()==source){++changed;auto c=replacement;c.setAlpha(source.alpha());return QBrush(c);}
            return brush;
        }
    } engine;
    QPaintDevice *reference;bool nativeMetrics=false;
};
}
