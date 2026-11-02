// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QWheelEvent>
#include <QTimer>
#include <QScreen>
#include <QCursor>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <functional>
#include "timelinecursorrelay.h"
#include "timelinepointercoordinates.h"
#include "timelineviewportdragdispatch.h"

namespace Unbound {
// Route presentation-local input to the original native viewport/child. Native
// scene handlers retain selection, editing, drag/drop, keyboard and command state.
class TimelineInputRouter : public QObject {
public:
    TimelineInputRouter(QWidget *display, QWidget *nativeViewport,
                        std::function<QPointF(QPointF)> mapping,
                        std::function<double()> scale)
        : QObject(display), display(display), native(nativeViewport), mapping(std::move(mapping)), scale(std::move(scale)), cursors(display), coordinates(display,nativeViewport,this->mapping) {
        display->installEventFilter(this);
        display->setFocusPolicy(Qt::StrongFocus);
        display->setFocusProxy(nativeViewport);
        display->setMouseTracking(true);
        display->setAcceptDrops(true);
    }
    void geometryChanged(){coordinates.geometryChanged();}
    std::function<void(const QString &)> edited;
    // Align through native queued handlers; no command replay or OS pointer warp.
    bool dispatchPointerShortcut(QObject *receiver,QEvent *event) {
        if(!native || !display || !receiver)return false;
        if(!coordinates.alignCursor())return false;
        QPointer<TimelineInputRouter> alive=this;
        QApplication::sendEvent(receiver,event);
        if(alive && edited)edited("native-timeline-shortcut");
        return true;
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override {
        if (!native || forwarding)
            return false;
        switch (event->type()) {
        case QEvent::Hide:
            cursors.clear();coordinates.restore();return false;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
            return mouse(static_cast<QMouseEvent *>(event));
        case QEvent::Wheel: {
            auto wheel = static_cast<QWheelEvent *>(event);
            const auto point = mapping(wheel->position());
            auto target = targetAt(point);
            QWheelEvent mapped(local(target, point), native->mapToGlobal(point.toPoint()), wheel->pixelDelta() / scale(),
                               wheel->angleDelta(), wheel->buttons(), wheel->modifiers(), wheel->phase(),
                               wheel->inverted(), wheel->source());
            return send(target, mapped, event);
        }
        case QEvent::ContextMenu: {
            auto menu = static_cast<QContextMenuEvent *>(event);
            const auto point = mapping(menu->pos());
            auto target = targetAt(point);
            const auto sourceGlobal=native->mapToGlobal(point.toPoint());
            placePopup(menu->globalPos(),sourceGlobal);
            QContextMenuEvent mapped(menu->reason(), local(target, point).toPoint(), sourceGlobal, menu->modifiers());
            return send(target, mapped, event);
        }
        case QEvent::ToolTip: {
            auto help = static_cast<QHelpEvent *>(event);
            const auto point = mapping(help->pos());
            auto target = targetAt(point);
            const auto sourceGlobal=native->mapToGlobal(point.toPoint());
            placePopup(help->globalPos(),sourceGlobal,true);
            QHelpEvent mapped(QEvent::ToolTip, local(target, point).toPoint(), sourceGlobal);
            return send(target, mapped, event);
        }
        case QEvent::DragEnter:case QEvent::DragMove:case QEvent::Drop:case QEvent::DragLeave: {
            QPointer<TimelineInputRouter> alive=this;forwarding=true;
            TimelineViewportDragDispatch::dispatch(display,native,event,mapping,
                [this](QPointF point,QPoint physical){coordinates.alignNative(point,physical);});
            if(!alive)return true;forwarding=false;cursors.refresh();
            if(edited)edited("native-timeline-"+TimelineViewportDragDispatch::phase(event->type()));
            return true;
        }
        case QEvent::Leave:
            cursors.clear();coordinates.restore();
            return send(native,*event,event);
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
            return send(native,*event,event);
        default:
            return false;
        }
    }
private:
    QWidget *targetAt(QPointF point) const {
        auto child = native->childAt(point.toPoint());
        return child ? child : native.data();
    }
    QPointF local(QWidget *target, QPointF point) const {
        return target == native ? point : point - target->mapTo(native, QPoint());
    }
    bool send(QWidget *target, QEvent &mapped, QEvent *original) {
        QPointer<TimelineInputRouter> alive=this;
        forwarding = true;
        QApplication::sendEvent(target, &mapped);
        if(!alive)return true;
        forwarding = false;
        cursors.refresh();
        if(placingPopup) {
            expectedPopup=mapped.isAccepted()?(placingTooltip?tooltip():QApplication::activePopupWidget()):nullptr;
            placingPopup=false;
        }
        original->setAccepted(mapped.isAccepted());
        const bool changing=mapped.type()!=QEvent::MouseMove || static_cast<QMouseEvent &>(mapped).buttons()!=Qt::NoButton;
        if(edited && changing && mapped.type()!=QEvent::ToolTip && mapped.type()!=QEvent::Leave)edited("native-timeline-input");
        return true;
    }
    bool mouse(QMouseEvent *event) {
        const bool start=event->type()==QEvent::MouseButtonPress || event->type()==QEvent::MouseButtonDblClick;
        if(event->type()==QEvent::MouseMove && event->buttons()==Qt::NoButton)pressed=nullptr;
        if(start) {
            pressPoint=event->localPos();pressSource=mapping(pressPoint);
            horizontal=mapping(pressPoint+QPointF(1,0))-pressSource;
            vertical=mapping(pressPoint+QPointF(0,1))-pressSource;
        }
        // Keep the press surface's affine map through a grab. A scrollbar drag
        // or trim remains continuous when the pointer leaves its painted band.
        const auto delta=event->localPos()-pressPoint;
        const auto point=(start||pressed)?pressSource+horizontal*delta.x()+vertical*delta.y():mapping(event->localPos());
        coordinates.alignNative(point,event->screenPos().toPoint());
        auto target = pressed ? pressed.data() : targetAt(point);
        if (start) {
            pressed = target;
            placePopup(event->screenPos().toPoint(),native->mapToGlobal(point.toPoint()));
            if (target->focusPolicy() != Qt::NoFocus)
                target->setFocus(Qt::MouseFocusReason);
            else
                native->setFocus(Qt::MouseFocusReason);
        }
        if(event->type()==QEvent::MouseButtonRelease)
            placePopup(event->screenPos().toPoint(),native->mapToGlobal(point.toPoint()));
        const auto position = local(target, point);
        cursors.track(native,point);
        QPointer<TimelineInputRouter> alive=this;
        QMouseEvent mapped(event->type(), position, position + target->mapTo(target->window(), QPoint()),
                           native->mapToGlobal(point.toPoint()), event->button(), event->buttons(), event->modifiers(), event->source());
        const bool result = send(target, mapped, event);
        if(!alive)return result;
        if (event->type() == QEvent::MouseButtonRelease && event->buttons() == Qt::NoButton)
            pressed = nullptr;
        return result;
    }
    static QWidget *tooltip() {
        for(auto widget:QApplication::topLevelWidgets())
            if(widget->isVisible() && widget->windowType()==Qt::ToolTip)return widget;
        return nullptr;
    }
    void placePopup(QPoint physical,QPoint source,bool help=false) {
        const auto token=++popupToken;
        const auto shift=physical-source;
        QPointer<QWidget> before=help?tooltip():QApplication::activePopupWidget();
        placingPopup=true;placingTooltip=help;expectedPopup=nullptr;
        // Qt hit testing needs a source-consistent screen position. Relocate
        // only the popup produced by this dispatch back to the displayed point.
        // A native exec() menu runs this once in its own nested event loop.
        QTimer::singleShot(0,this,[this,token,shift,physical,before,help] {
            if(token!=popupToken)return;
            auto popup=help?tooltip():QApplication::activePopupWidget();
            if(!popup || (!help && popup==before) || (!placingPopup && popup!=expectedPopup))return;
            auto position=popup->pos()+shift;
            if(auto screen=QGuiApplication::screenAt(physical)) {
                const auto bounds=screen->availableGeometry();
                position.setX(qBound(bounds.left(),position.x(),qMax(bounds.left(),bounds.right()-popup->width()+1)));
                position.setY(qBound(bounds.top(),position.y(),qMax(bounds.top(),bounds.bottom()-popup->height()+1)));
            }
            popup->move(position);
        });
    }
    QPointer<QWidget> display, native, pressed;
    std::function<QPointF(QPointF)> mapping;
    std::function<double()> scale;
    TimelineCursorRelay cursors;
    TimelinePointerCoordinates coordinates;
    bool forwarding = false;
    bool placingPopup=false,placingTooltip=false;
    quint64 popupToken=0;
    QPointer<QWidget> expectedPopup;
    QPointF pressPoint,pressSource,horizontal,vertical;
};
}
