// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QScrollBar>
#include <functional>
#include <QMap>
#include "nativepaintfixtures.h"

class UiTiClipContainer : public QGraphicsObject {
    Q_OBJECT
public:
    QRectF bounds{0,0,240,95};QColor fill{30,130,180};int presses=0;bool rounded=false,rasterOnly=false,paintBody=true;
    UiTiClipContainer(){setFlags(ItemIsSelectable|ItemIsFocusable);}
    QRectF boundingRect() const override {return bounds;}
    void setWidth(double w){prepareGeometryChange();bounds.setWidth(w);update();}
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {
        if(rasterOnly){QImage image(bounds.size().toSize(),QImage::Format_RGB32);image.fill(fill);p->drawImage(bounds,image);return;}
        if(paintBody){if(rounded){p->setPen(Qt::NoPen);p->setBrush(fill);p->drawRoundedRect(bounds,5,5);}else p->fillRect(bounds,fill);}
        QImage image(40,25,QImage::Format_RGB32);image.fill(fill);image.setPixelColor(15,15,Qt::yellow);
        p->drawImage(QPointF(10,5),image);
        p->fillRect(QRectF(5,35,170,55),Qt::black);
        QPainterPath waveform;waveform.moveTo(10,55);waveform.lineTo(20,40);
        waveform.lineTo(30,55);waveform.lineTo(40,42);waveform.lineTo(50,55);waveform.closeSubpath();
        p->setPen(Qt::NoPen);p->setBrush(fill);p->drawPath(waveform);
        p->setPen(fill);QFont f;f.setPixelSize(13);p->setFont(f);p->drawText(QPointF(12,80),"Native clip text");
        f.setPointSize(9);p->setFont(f);p->drawText(QPointF(120,80),"Point size");
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override {++presses;QGraphicsObject::mousePressEvent(e);}
};

// Native capture shows body batches in drawBackground, outside clip-item paint.
class BatchedClipSceneView : public QGraphicsView {
public:
    using QGraphicsView::QGraphicsView;
    bool clipBackground=false,clipBatchToExposure=false;
protected:
    void drawBackground(QPainter *p,const QRectF &area) override {
        QGraphicsView::drawBackground(p,area);if(!clipBackground || !scene())return;
        QMap<QRgb,QVector<QRectF>> batches;
        for(auto item:scene()->items())if(!item->parentItem() && item->flags().testFlag(QGraphicsItem::ItemHasNoContents))
            if(auto clip=dynamic_cast<UiTiClipContainer *>(item))if(clip->isVisible())
                batches[clip->fill.rgba()].append(clipBatchToExposure?clip->sceneBoundingRect().intersected(area):clip->sceneBoundingRect());
        p->save();p->setPen(Qt::NoPen);
        for(auto it=batches.cbegin();it!=batches.cend();++it){p->setBrush(QColor::fromRgba(it.key()));p->drawRects(it.value());}
        p->restore();
    }
};

// Native semantic types use real Qt metadata, rather than production test flags.
class UiTiTrackControlItem : public QGraphicsObject {
    Q_OBJECT
public:
    QRectF boundingRect() const override {return {0,0,260,70};}
    UiTiTrackControlItem(){setZValue(-100);setAcceptedMouseButtons(Qt::NoButton);}
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) override {
        p->fillRect(boundingRect(),QColor(35,35,38));p->setPen(Qt::black);p->drawLine(QPointF(0,69),QPointF(260,69));
    }
};
class UiTiTrackLayerItem : public QGraphicsObject {
    Q_OBJECT
public:
    QColor color;
    int clicks = 0;
    explicit UiTiTrackLayerItem(QColor color) : color(color) {}
    QRectF boundingRect() const override { return {0, 0, 32, 20}; }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override {
        p->fillRect(boundingRect(), color);
        const auto at = p->worldTransform().map(QPointF(7, 15));
        p->save(); p->setWorldTransform(QTransform());
        QFont font; font.setPixelSize(12); p->setFont(font); p->setPen(Qt::white);
        p->drawText(at, "V1"); p->restore();
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override { ++clicks; event->accept(); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override { event->accept(); }
};

class UiTiScrollBarItem : public QGraphicsObject {
    Q_OBJECT
public:
    QRectF bounds;
    QPointF lastClick;
    void setBounds(QRectF value) { prepareGeometryChange(); bounds = value; update(); }
    QRectF boundingRect() const override { return bounds; }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override { p->fillRect(bounds, QColor(80, 80, 80)); }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override { lastClick = event->scenePos(); event->accept(); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override { event->accept(); }
};

namespace OpenGateTest {
class ClickRect : public QGraphicsRectItem {
public:
    int clicks = 0;
    int contexts = 0;
    int moves = 0;
    QPointF lastDrag;
    std::function<void(QPoint)> showContext;
    explicit ClickRect(QRectF rect) : QGraphicsRectItem(rect) { setPen(QPen(Qt::cyan)); }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override { ++clicks; event->accept(); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override { event->accept(); }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override { ++moves;lastDrag=event->scenePos();event->accept(); }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override {
        ++contexts;
        if(showContext)showContext(event->screenPos());
        event->accept();
    }
};

class TimelineSourceView : public QGraphicsView {
public:
    QGraphicsScene *sequence, *controlsScene, *tracksScene;
    QGraphicsProxyWidget *controlsProxy, *tracksProxy;
    UiTiTrackLayerItem *blue, *red;
    UiTiScrollBarItem *bar = nullptr;
    ClickRect *clip, *lastControl;
    QPointF lastMouse, lastWheel;
    QPoint lastContextGlobal;
    explicit TimelineSourceView(QWidget *owner) : QGraphicsView(owner) {
        setObjectName("m_TiTimelineView");
        sequence = new QGraphicsScene(this);
        controlsScene = new QGraphicsScene(this);
        tracksScene = new QGraphicsScene(this);
        setScene(sequence); setup(this);
        controlsScene->setSceneRect(0, 0, 260, 400);
        tracksScene->setSceneRect(0, 0, 4000, 400);
        auto controls = new QGraphicsView(controlsScene);
        controls->setObjectName("VideoTrackControlsView"); setup(controls);
        auto tracks = new BatchedClipSceneView(tracksScene);
        tracks->setObjectName("VideoTracksView"); setup(tracks);
        controlsProxy = sequence->addWidget(controls);
        tracksProxy = sequence->addWidget(tracks);
        for (int row = 0; row < 3; ++row) {
            auto track=new UiTiTrackControlItem;track->setPos(0,row*70);controlsScene->addItem(track);
            auto b = new UiTiTrackLayerItem(QColor(20, 90, 180));
            auto r = new UiTiTrackLayerItem(QColor(150, 30, 30));
            b->setPos(15, row * 70 + 3); r->setPos(54, row * 70 + 3);
            controlsScene->addItem(b); controlsScene->addItem(r);
            if (row == 0) { blue = b; red = r; }
            auto name = controlsScene->addSimpleText("Whole track name");
            name->setBrush(Qt::white); name->setPos(18, row * 70 + 32);
        }
        lastControl = new ClickRect({220, 3, 26, 20});
        controlsScene->addItem(lastControl);
        auto glyph = new FixedGlyphItem;
        glyph->setPos(10, 15); tracksScene->addItem(glyph);
        clip = new ClickRect({0, 0, 200, 100}); clip->setZValue(3); tracksScene->addItem(clip);
        bar = new UiTiScrollBarItem; sequence->addItem(bar);
        auto clock=sequence->addSimpleText("00:00:00:00");clock->setBrush(QColor(255,180,0));clock->setPos(60,8);
        reflow();
    }
    static void setup(QGraphicsView *view) {
        view->setFrameShape(QFrame::NoFrame);
        view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
protected:
    void resizeEvent(QResizeEvent *event) override { QGraphicsView::resizeEvent(event); if (bar) reflow(); }
    void mousePressEvent(QMouseEvent *event) override { lastMouse = event->localPos(); QGraphicsView::mousePressEvent(event); }
    void wheelEvent(QWheelEvent *event) override { lastWheel = event->position(); event->accept(); }
    void contextMenuEvent(QContextMenuEvent *event) override { lastContextGlobal = event->globalPos(); QGraphicsView::contextMenuEvent(event); }
private:
    void reflow() {
        const int w = viewport()->width(), h = viewport()->height();
        controlsProxy->setGeometry(QRectF(0, 40, 260, qMax(1, h - 52)));
        tracksProxy->setGeometry(QRectF(260, 40, qMax(1, w - 260), qMax(1, h - 52)));
        bar->setBounds(QRectF(0, qMax(0, h - 12), w, 12));
        sequence->setSceneRect(0, 0, w, h);
    }
};

struct TimelineViewportFixture {
    QWidget timeline;
    QVBoxLayout *layout;
    QPushButton *toolbar;
    TimelineSourceView *view;
    explicit TimelineViewportFixture(QWidget *owner=nullptr) : timeline(owner) {
        timeline.setObjectName("frameConformTimelines");
        if(!owner)timeline.setAttribute(Qt::WA_DontShowOnScreen);
        layout = new QVBoxLayout(&timeline); layout->setContentsMargins(0, 0, 0, 0);
        toolbar = new QPushButton("Native timeline tools"); toolbar->setFixedHeight(31); layout->addWidget(toolbar);
        view = new TimelineSourceView(&timeline); layout->addWidget(view);
        timeline.resize(900, 600); timeline.show(); settle();
        for (auto proxy : {view->controlsProxy, view->tracksProxy}) {
            auto nested = qobject_cast<QGraphicsView *>(proxy->widget());
            nested->horizontalScrollBar()->setValue(nested->horizontalScrollBar()->minimum());
            nested->verticalScrollBar()->setValue(nested->verticalScrollBar()->minimum());
        }
        settle();
    }
    static void settle() { for (int i = 0; i < 12; ++i) QApplication::processEvents(); }
    static void click(QWidget *widget, QPointF point) {
        const auto global = widget->mapToGlobal(point.toPoint());
        QMouseEvent down(QEvent::MouseButtonPress, point, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &down);
        QMouseEvent up(QEvent::MouseButtonRelease, point, global, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &up);
        settle();
    }
};
}
