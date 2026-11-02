// SPDX-License-Identifier: MIT
#pragma once
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QtCore/qscopeguard.h>
#include "timelinescenerenderer.h"
#include "timelinetrackheaderlayout.h"
#include "timelinetrackheadercontrols.h"
#include "timelineinputrouter.h"
#include "timelinesceneupdates.h"
#include "timelinecommandpaintevidence.h"

namespace Unbound {
// Present one live native Timeline view. The original scene/model/input handlers
// remain in place; only backing resolution and the header's presentation change.
class TimelineContentViewport : public QWidget {
public:
    TimelineContentViewport(QGraphicsView *nativeView, QWidget *owner)
        : QWidget(owner), view(nativeView), layer(new Layer(this)), controls(layer) {
        setObjectName("openGateTimelineContentViewport");
        setProperty("openGateChrome", true);
        setMinimumSize(0, 0);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        layer->setObjectName("openGateTimelineContentLayer");
        // An opaque native child remains above the original GPU viewport.
        layer->setAttribute(Qt::WA_NativeWindow);
        layer->setAttribute(Qt::WA_OpaquePaintEvent);
        nativeView->setParent(this);
        nativeView->show();
        input = new TimelineInputRouter(layer, view->viewport(), [this](QPointF point) {
            return geometry.toNative(point / factor);
        }, [this] { return factor; });
        controls.divider->moved = [this](double delta) {
            requestedWidth = geometry.nominalWidth/geometry.ratio + delta / headerScale();
            arrange();
            requestedWidth = geometry.nominalWidth/geometry.ratio;
            notify();
        };
        connect(controls.bar, &QScrollBar::valueChanged, this, [this](int value) {
            requestedOffset = value / headerScale();
            arrange();
            notify();
        });
        connect(controls.pin, &QToolButton::toggled, this, [this](bool value) {
            pinned = value;
            arrange();
            notify();
        });
        controls.value=[this]{return headerScale();};controls.maximum=[this]{return factor;};
        controls.set=[this](double value) {
            if(value!=0. && (!qIsFinite(value) || value<.7 || value>factor))return;
            requestedHeaderScale=value;arrange();notify();
        };
        updates=new TimelineSceneUpdates(view,this);
        updates->suspended=[this]{if(!rendering)renderer.releasePixels();};
        updates->capturing=[this]{return rendering;};
        updates->headerMovesWithTime=[this]{return !pinned;};
        updates->commandObserved=[this](const QString &source){evidence.requested(source,renderer.revision());};
        updates->layoutChanged=[this]{if(!arranging && !rendering)arrange();};
        updates->damaged=[this](const QList<QRectF> &rects) {
            if(rects.isEmpty()){renderer.invalidateAll();updates->schedulePaint(QRegion(layer->rect()));return;}
            QRegion visible;
            for(const auto &rect:rects) {
                if(!rect.isValid()){renderer.invalidateAll();updates->schedulePaint(QRegion(layer->rect()));return;}
                renderer.invalidate(rect);
                visible+=geometry.presentationDamage(view->viewportTransform().mapRect(TimelineSceneRenderer::damageBounds(rect)),factor);
            }
            if(!visible.isEmpty())updates->schedulePaint(visible);
        };
        updates->repaint=[this]{if(!arranging)updates->schedulePaint(QRegion(layer->rect()));};
        updates->paintReady=[this](const QRegion &region){layer->update(region);};
        input->edited=[this](const QString &source){updates->commandCompleted();if(updates->commandObserved)updates->commandObserved(source);};
        updates->refresh();
        layer->show();
    }
    ~TimelineContentViewport() override {
        updates->stop();
        input->edited={};
        disconnect(controls.bar,nullptr,this,nullptr);
        disconnect(controls.pin,nullptr,this,nullptr);
        controls.retire();changed={};layer->retire();
    }
    std::function<void()> changed;
    void configure(double value, double width, double offset, bool pin,double header=0.) {
        factor = value;
        requestedWidth = width;
        requestedOffset = offset;
        pinned = pin;
        requestedHeaderScale=header;
        arrange();
    }
    double headerWidth() const { return geometry.width; }
    double retainedHeaderWidth() const { return requestedWidth; }
    double headerOffset() const { return geometry.offset; }
    bool headerPinned() const { return pinned; }
    double headerScalePreference() const {return requestedHeaderScale;}
    double headerScale() const {return requestedHeaderScale>0.?qMin(requestedHeaderScale,factor):factor;}
    QGraphicsView *nativeView() const { return view; }
    QWidget *presentation() const { return layer; }
    bool dispatchPointerShortcut(QObject *receiver,QEvent *event) {
        return input->dispatchPointerShortcut(receiver,event);
    }
protected:
    void resizeEvent(QResizeEvent *event) override { QWidget::resizeEvent(event); arrange(); }
private:
    class Layer : public QWidget {
    public:
        explicit Layer(TimelineContentViewport *owner) : QWidget(owner), owner(owner) {}
        void retire() { owner=nullptr;hide(); }
    protected:
        void paintEvent(QPaintEvent *event) override { if(owner)owner->paint(this,event->region()); }
    private:
        TimelineContentViewport *owner;
    };
    void notify() { if (changed) changed(); }
    void arrange() {
        if(rendering){if(updates)updates->geometryInvalidated();return;}
        if (arranging || !view || !view->scene() || width() <= 0 || height() <= 0)
            return;
        QScopedValueRollback<bool> guard(arranging, true);
        layer->setGeometry(rect());
        geometry = TimelineTrackHeaderLayout::discover(view);
        if (geometry.nativeWidth <= 0.) {
            view->setGeometry(0, 0, qMax(1, qRound(width() / factor)), qMax(1, qRound(height() / factor)));
            geometry = TimelineTrackHeaderLayout::discover(view);
        }
        if(geometry.nativeWidth>0.) {
            const double headerMinimum=(pinned?controls.minimumWidth:controls.footerControlsWidth)/factor;
            setMinimumSize(qCeil((headerMinimum+geometry.minimumBodyWidth)*factor),qCeil(geometry.minimumHeight*factor));
            layer->setGeometry(rect());
        } else setMinimumSize(0,0);
        configureGeometry();
        const auto frame = view->size() - view->viewport()->size();
        const int nativeWidth = qMax(1, qCeil(width() / factor + geometry.nativeWidth - geometry.width)) + frame.width();
        QList<QPair<QPointer<QGraphicsView>, QGraphicsView::ViewportAnchor>> anchors;
        anchors.append({view, view->resizeAnchor()});
        for (auto item : view->scene()->items()) {
            auto proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
            auto nested = proxy ? qobject_cast<QGraphicsView *>(proxy->widget()) : nullptr;
            if (nested) anchors.append({nested, nested->resizeAnchor()});
        }
        for (const auto &entry : anchors) if (entry.first) entry.first->setResizeAnchor(QGraphicsView::NoAnchor);
        const double nativeFooter=geometry.horizontalBar.isEmpty()?0.:geometry.horizontalBar.height();
        view->setGeometry(0, 0, nativeWidth,
            qMax(1,qCeil((height()-controls.footerHeight)/factor+nativeFooter))+frame.height());
        for (const auto &entry : anchors) if (entry.first) entry.first->setResizeAnchor(entry.second);
        geometry = TimelineTrackHeaderLayout::discover(view);
        configureGeometry();
        const int barTop = qMax(0,height()-controls.footerHeight);
        controls.arrange(qRound(geometry.width * factor), height(), barTop, headerScale(), geometry.offset,
                         geometry.maximumOffset(), pinned);
        updates->geometryObserved();
        input->geometryChanged();
        layer->raise();
        layer->update();
    }
    void configureGeometry() {
        geometry.configure(QSizeF(width()/factor,height()/factor),requestedWidth,requestedOffset,pinned,
            controls.footerHeight/factor,headerScale()/factor,controls.minimumWidth/factor,controls.footerControlsWidth/factor);
    }
    void paint(QWidget *surface,const QRegion &region) {
        if(rendering){updates->schedulePaint(region);return;}
        updates->presented(region);
        QPainter painter(surface);
        painter.fillRect(surface->rect(), palette().window());
        if (!view || !view->scene() || rendering)
            return;
        const auto resume=qScopeGuard([this]{updates->captureFinished();});
        QScopedValueRollback<bool> guard(rendering, true);
        const auto source = view->mapToScene(view->viewport()->rect()).boundingRect();
        const double backing = factor * surface->devicePixelRatioF();
        if(!renderer.render(view->scene(),source,backing,geometry.sectionTops))return;
        if(evidence.waiting() && !renderer.pendingDamage())evidence.presented(renderer.revision(),renderer.sampledFingerprint(),factor,headerScale(),view);
        for(const auto &slice:geometry.slices()) {
            const QRectF destination(slice.target.topLeft()*factor,slice.target.size()*factor);
            if(slice.background){painter.fillRect(destination,renderer.backgroundColor(slice.source,palette().window().color()));continue;}
            painter.setRenderHint(QPainter::SmoothPixmapTransform,slice.target.width()<slice.source.width());
            renderer.draw(painter,slice.source,destination);
        }
        for(auto editor:view->viewport()->findChildren<QWidget *>()) {
            if(!editor->isVisible() || !editor->inherits("QLineEdit"))continue;
            updates->watchEditor(editor);
            QImage image(QSize(qCeil(editor->width()*backing),qCeil(editor->height()*backing)),QImage::Format_ARGB32_Premultiplied);
            if(image.isNull())continue;
            image.setDevicePixelRatio(backing);image.fill(Qt::transparent);
            {QPainter native(&image);editor->render(&native);}
            const QRectF bounds(editor->mapTo(view->viewport(),QPoint()),editor->size());
            for(const auto &slice:geometry.slices()) {
                const auto hit=bounds.intersected(slice.source);if(hit.isEmpty())continue;
                const double x=slice.target.width()/slice.source.width(),y=slice.target.height()/slice.source.height();
                const auto at=slice.target.topLeft()+QPointF((hit.left()-slice.source.left())*x,(hit.top()-slice.source.top())*y);
                painter.drawImage(QRectF(at*factor,QSizeF(hit.width()*x*factor,hit.height()*y*factor)),image,
                    QRectF((hit.topLeft()-bounds.topLeft())*backing,hit.size()*backing));
            }
        }
    }
    QPointer<QGraphicsView> view;
    TimelineSceneRenderer renderer;
    TimelineCommandPaintEvidence evidence;
    TimelineSceneUpdates *updates = nullptr;
    Layer *layer;
    TimelineInputRouter *input = nullptr;
    TimelineTrackHeaderControls controls;
    TimelineTrackHeaderLayout geometry;
    double factor = 1., requestedWidth = 0., requestedOffset = 0.,requestedHeaderScale=0.;
    bool pinned = true, arranging = false, rendering = false;
};
}
