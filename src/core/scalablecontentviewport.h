// SPDX-License-Identifier: MIT
#pragma once
#include <QtMath>
#include <QGraphicsView>
#include <QPointer>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QScrollBar>
#include <QResizeEvent>
#include <QWindow>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
namespace Unbound {
// Live native QWidget rendering with Qt's inverse input mapping and real scrollbars.
// The canvas keeps its logical minimum; the outer viewport remains freely resizable.
class ScalableContentViewport : public QGraphicsView {
public:
    ScalableContentViewport(QWidget *content,QWidget *owner,int logicalWidth=0):QGraphicsView(owner),content(content) {
        setObjectName("openGateScalableContentViewport");setProperty("openGateChrome",true);
        setFrameShape(QFrame::NoFrame);setMinimumSize(0,0);setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
        setAlignment(Qt::AlignLeft|Qt::AlignTop);setTransformationAnchor(NoAnchor);setResizeAnchor(NoAnchor);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        canvasWidth=qMax(logicalWidth>0?logicalWidth:content->width(),qMax(1,content->minimumSizeHint().width()));
        minimumHeight=qMax(1,content->minimumSizeHint().height());
        originalOffscreen=content->testAttribute(Qt::WA_DontShowOnScreen);originalQuit=content->testAttribute(Qt::WA_QuitOnClose);
        originalPopupBypass=content->windowFlags().testFlag(Qt::BypassGraphicsProxyWidget);
        auto scene=new QGraphicsScene(this);setScene(scene);proxy=new ContentProxy;
        proxy->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
        scene->addItem(proxy);content->setAttribute(Qt::WA_DontShowOnScreen);content->setParent(nullptr);
        // Explicitly embed only the canvas. Descendant menus/dialogs must keep
        // native popup tracking and screen bounds, not become clipped/scaled
        // child graphics proxies. Qt checks this flag along popup ancestry.
        content->setWindowFlag(Qt::BypassGraphicsProxyWidget,true);
        proxy->setWidget(content);proxy->setFocusPolicy(Qt::StrongFocus);content->show();
        fitCanvas();
    }
    int logicalCanvasWidth() const {return canvasWidth;}
    void restoreCanvasWidth(int width) {if(width>0){canvasWidth=width;fitCanvas();}}
    bool owns(QWidget *widget) const {return proxy && proxy->widget()==widget;}
    void setContentScale(double factor) {
        amount=factor;setTransform(QTransform::fromScale(factor,factor));fitCanvas();
    }
    QWidget *release(QWidget *owner) {
        if(!proxy || !content)return nullptr;
        auto widget=content.data();widget->hide();
        // Retire the proxy's temporary widget window before child reparenting;
        // no host/main window is involved in this transition.
        if(widget->windowHandle())widget->windowHandle()->destroy();
        widget->setParent(owner,Qt::Widget);proxy->setWidget(nullptr);
        widget->setWindowFlag(Qt::BypassGraphicsProxyWidget,originalPopupBypass);
        widget->setAttribute(Qt::WA_DontShowOnScreen,originalOffscreen);widget->setAttribute(Qt::WA_QuitOnClose,originalQuit);
        content=nullptr;return widget;
    }
protected:
    void resizeEvent(QResizeEvent *event) override {QGraphicsView::resizeEvent(event);fitCanvas();}
private:
    // Keep Qt's native proxy focus, hit testing and popup integration, but paint
    // custom native delegates at backing resolution instead of stretching their
    // internal painter coordinates. Render only the exposed canvas region.
    class ContentProxy : public QGraphicsProxyWidget {
    public:
        void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *) override {
            auto content=widget();if(!content)return;
            const auto exposed=option->exposedRect.intersected(boundingRect()).toAlignedRect();
            const double backing=QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform())*
                                 painter->device()->devicePixelRatioF();
            if(exposed.isEmpty() || backing<=0. || exposed.width()*backing>16384 || exposed.height()*backing>16384 ||
               exposed.width()*exposed.height()*backing*backing>64.*1024*1024)return;
            QImage image(QSize(qCeil(exposed.width()*backing),qCeil(exposed.height()*backing)),QImage::Format_ARGB32_Premultiplied);
            if(image.isNull())return;
            image.setDevicePixelRatio(backing);image.fill(Qt::transparent);
            {
                QPainter nativePainter(&image);
                content->render(&nativePainter,QPoint(),QRegion(exposed));
            }
            painter->drawImage(QRectF(exposed),image,QRectF(QPointF(),image.size()));
        }
    };
    void fitCanvas() {
        if(!proxy || !content)return;
        const auto width=qMax(canvasWidth,int(viewport()->width()/amount));
        const auto height=qMax(minimumHeight,int(viewport()->height()/amount));
        proxy->resize(width,height);setSceneRect(proxy->boundingRect());
    }
    QPointer<QWidget> content;
    QGraphicsProxyWidget *proxy=nullptr;
    int canvasWidth=1,minimumHeight=1;double amount=1.;bool originalOffscreen=false,originalQuit=false,originalPopupBypass=false;
};
}
