// SPDX-License-Identifier: MIT
#pragma once
#include <QResizeEvent>
#include <QScopedValueRollback>
#include "timelinescenerenderer.h"
#include "timelinetrackheaderlayout.h"
#include "timelinetrackheadercontrols.h"
#include "timelineinputrouter.h"

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
            requestedWidth = geometry.width + delta / factor;
            arrange();
            requestedWidth = geometry.width;
            notify();
        };
        connect(controls.bar, &QScrollBar::valueChanged, this, [this](int value) {
            requestedOffset = value / factor;
            arrange();
            notify();
        });
        connect(controls.pin, &QToolButton::toggled, this, [this](bool value) {
            pinned = value;
            arrange();
            notify();
        });
        observeScene();
        view->installEventFilter(this);
        view->viewport()->installEventFilter(this);
        layer->show();
    }
    ~TimelineContentViewport() override {
        if(scene)disconnect(scene,nullptr,this,nullptr);
        disconnect(controls.bar,nullptr,this,nullptr);
        disconnect(controls.pin,nullptr,this,nullptr);
        controls.divider->moved={};changed={};
        for(auto editor:editors)if(editor)editor->removeEventFilter(this);
        layer->retire();
        if (view) {
            view->removeEventFilter(this);
            if (view->viewport()) view->viewport()->removeEventFilter(this);
        }
    }
    std::function<void()> changed;
    void configure(double value, double width, double offset, bool pin) {
        factor = value;
        requestedWidth = width;
        requestedOffset = offset;
        pinned = pin;
        arrange();
    }
    double headerWidth() const { return geometry.width; }
    double retainedHeaderWidth() const { return requestedWidth; }
    double headerOffset() const { return geometry.offset; }
    bool headerPinned() const { return pinned; }
    QGraphicsView *nativeView() const { return view; }
    QWidget *presentation() const { return layer; }
    bool dispatchPointerShortcut(QObject *receiver,QEvent *event) {
        return input->dispatchPointerShortcut(receiver,event);
    }
protected:
    void resizeEvent(QResizeEvent *event) override { QWidget::resizeEvent(event); arrange(); }
    bool eventFilter(QObject *, QEvent *event) override {
        if (!arranging && !rendering && (event->type() == QEvent::Paint || event->type() == QEvent::Show ||
            event->type() == QEvent::LayoutRequest || event->type() == QEvent::ChildAdded ||
            event->type() == QEvent::KeyPress || event->type() == QEvent::InputMethod ||
            event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
            observeScene();
            layer->update();
        }
        return false;
    }
private:
    class Layer : public QWidget {
    public:
        explicit Layer(TimelineContentViewport *owner) : QWidget(owner), owner(owner) {}
        void retire() { owner=nullptr;hide(); }
    protected:
        void paintEvent(QPaintEvent *) override { if(owner)owner->paint(this); }
    private:
        TimelineContentViewport *owner;
    };
    void observeScene() {
        if (!view || scene == view->scene())
            return;
        if (scene) disconnect(scene, nullptr, this, nullptr);
        scene = view->scene();
        if (scene) connect(scene, &QGraphicsScene::changed, this, [this] {
            if (!rendering) arrange();
        });
    }
    void notify() { if (changed) changed(); }
    void arrange() {
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
            const double headerMinimum=pinned?geometry.pinnedWidth+24.:40.;
            setMinimumSize(qCeil((headerMinimum+geometry.minimumBodyWidth)*factor),qCeil(geometry.minimumHeight*factor));
            layer->setGeometry(rect());
        } else setMinimumSize(0,0);
        geometry.configure(QSizeF(width() / factor, height() / factor), requestedWidth, requestedOffset, pinned, controls.footerHeight/factor);
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
        geometry.configure(QSizeF(width() / factor, height() / factor), requestedWidth, requestedOffset, pinned, controls.footerHeight/factor);
        const int barTop = qMax(0,height()-controls.footerHeight);
        controls.arrange(qRound(geometry.width * factor), height(), barTop, factor, geometry.offset,
                         geometry.maximumOffset(), pinned, geometry.pinnedWidth);
        layer->raise();
        layer->update();
    }
    void paint(QWidget *surface) {
        QPainter painter(surface);
        painter.fillRect(surface->rect(), palette().window());
        if (!view || !view->scene() || rendering)
            return;
        QScopedValueRollback<bool> guard(rendering, true);
        geometry = TimelineTrackHeaderLayout::discover(view);
        geometry.configure(QSizeF(width() / factor, height() / factor), requestedWidth, requestedOffset, pinned, controls.footerHeight/factor);
        const auto source = view->mapToScene(view->viewport()->rect()).boundingRect();
        const double backing = factor * surface->devicePixelRatioF();
        auto image = renderTimelineScene(view->scene(), source, backing, geometry.sectionTops);
        if (image.isNull())
            return;
        {
            QPainter editorPainter(&image);
            for (auto editor : view->viewport()->findChildren<QWidget *>()) {
                if (!editor->isVisible() || !editor->inherits("QLineEdit"))
                    continue;
                if (!editors.contains(editor)) {
                    editors.append(editor);
                    editor->installEventFilter(this);
                }
                editor->render(&editorPainter, editor->mapTo(view->viewport(), QPoint()));
            }
        }
        for (const auto &slice : geometry.slices()) {
            if (slice.source.isEmpty() || slice.target.isEmpty())
                continue;
            const QRectF destination(slice.target.topLeft() * factor, slice.target.size() * factor);
            const QRectF pixels(slice.source.topLeft() * backing, slice.source.size() * backing);
            painter.drawImage(destination, image, pixels);
        }
    }
    QPointer<QGraphicsView> view;
    QPointer<QGraphicsScene> scene;
    QList<QPointer<QWidget>> editors;
    Layer *layer;
    TimelineInputRouter *input = nullptr;
    TimelineTrackHeaderControls controls;
    TimelineTrackHeaderLayout geometry;
    double factor = 1., requestedWidth = 0., requestedOffset = 0.;
    bool pinned = true, arranging = false, rendering = false;
};
}
