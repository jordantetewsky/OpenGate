// SPDX-License-Identifier: MIT
#pragma once
#include "nativepanelposition.h"
#include <QTimer>
#include "scalablecontentviewport.h"
#include "diagnostics.h"
namespace Unbound {
// Lease only Media Pool's bins/clip canvas. Native title and toolbar stay compact.
class MediaPoolContentScale {
public:
    ~MediaPoolContentScale(){restore();}
    std::function<void()> changed;
    int canvasWidth() const {return viewport?viewport->logicalCanvasWidth():savedWidth;}
    void restoreCanvasWidth(int width) {savedWidth=qBound(0,width,32768);if(viewport)viewport->restoreCanvasWidth(savedWidth);}
    QPoint offset() const { return viewport&&!pendingOffset?QPoint(viewport->horizontalScrollBar()->value(),viewport->verticalScrollBar()->value()):savedOffset; }
    void restoreOffset(QPoint value) { savedOffset=value;pendingOffset=true;scheduleOffset(); }
    static QWidget *canvas(QWidget *root) {
        QWidget *found=nullptr;if(!root)return nullptr;
        for(auto widget:root->findChildren<QWidget *>("mediaPoolViewSplitter")) {
            bool selected=true;
            for(auto parent=widget;parent&&parent!=root;parent=parent->parentWidget())if(parent->isHidden())selected=false;
            if(!selected)continue;
            if(found)return nullptr;
            found=widget;
        }
        return found;
    }
    static QWidget *unsupportedSurface(QWidget *widget) {
        if(!widget)return nullptr;
        auto widgets=widget->findChildren<QWidget *>();widgets.prepend(widget);
        for(auto child:widgets)if(!child->isWindow()&&child->testAttribute(Qt::WA_PaintOnScreen))return child;
        return nullptr;
    }
    static bool supported(QWidget *root) {
        if(!root)return false;
        if(root->findChild<QGraphicsView *>("openGateScalableContentViewport"))return true;
        auto widget=canvas(root);return widget&&!unsupportedSurface(widget);
    }
    bool apply(QWidget *root,double factor) {
        if(!root || !qIsFinite(factor) || factor<0.7 || factor>3.)return false;
        const auto wanted=pendingOffset?savedOffset:offset();
        if(!position.active()) {
            auto widget=canvas(root);if(!widget)return false;
            if(auto child=unsupportedSurface(widget)) {
                logEvent("panel.content.viewport.unavailable",{{"panel",root->objectName()},{"reason","native-paint-surface"},
                    {"class",child->metaObject()->className()},{"name",child->objectName()}});
                return false;
            }
            QPointer<QWidget> focus=QApplication::focusWidget();
            if(focus && focus!=widget && !widget->isAncestorOf(focus))focus=nullptr;
            const auto error=position.acquire(widget,root,false,true);if(!error.isEmpty())return false;
            auto slot=position.surface();auto layout=new QVBoxLayout(slot);layout->setContentsMargins(0,0,0,0);
            viewport=new ScalableContentViewport(widget,slot,savedWidth);
            if(!viewport->owns(widget)) {
                viewport->release(position.contentParking());delete viewport.data();viewport=nullptr;
                delete slot->layout();position.restore();return false;
            }
            layout->addWidget(viewport);viewport->show();slot->show();
            if(focus)focus->setFocus(Qt::OtherFocusReason);
            for(auto bar:{viewport->horizontalScrollBar(),viewport->verticalScrollBar()})
                QObject::connect(bar,&QScrollBar::actionTriggered,viewport,[this]{if(changed)changed();});
        }
        if(!viewport || !viewport->owns(position.content()))return false;
        viewport->setContentScale(factor);savedOffset=wanted;scheduleOffset();
        logEvent("panel.content.viewport.scale",{{"panel",root->objectName()},{"factor",factor},{"toolbar_scaled",false}});
        return true;
    }
    bool restore() {
        if(!position.active())return true;
        if(!viewport || !viewport->owns(position.content()))return false;
        savedOffset=offset();
        savedWidth=canvasWidth();
        pendingOffset=true;
        viewport->release(position.contentParking());
        auto slot=position.surface();
        delete viewport.data();
        viewport=nullptr;
        delete slot->layout();
        return position.restore();
    }

private:
    void scheduleOffset() {
        if(!viewport)return;
        QPointer<ScalableContentViewport> alive=viewport;const auto wanted=savedOffset;const auto version=++offsetVersion;
        QTimer::singleShot(0,viewport,[this,alive,wanted,version] {
            if(!alive || version!=offsetVersion)return;
            alive->horizontalScrollBar()->setValue(wanted.x());
            alive->verticalScrollBar()->setValue(wanted.y());
            pendingOffset=false;
        });
    }
    QPoint savedOffset;int savedWidth=0;bool pendingOffset=false;unsigned offsetVersion=0;
    NativePanelPosition position;QPointer<ScalableContentViewport> viewport;
};
}
