// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractScrollArea>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPointer>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Deliver one mapped drag phase through the original view's virtual viewport
// handler. QApplication::notify would retarget move/drop to the display again.
// The OS drag target, native scene handlers and MIME/source ownership stay intact.
// Root viewport QObject/application filters are not replayed here; the outer
// display event already passed application filters. Scene/proxy filters remain
// in the native virtual-handler path. Private host root filters need live proof.
class TimelineViewportDragDispatch {
public:
    using Map=std::function<QPointF(QPointF)>;
    using Align=std::function<void(QPointF,QPoint)>;
    static void dispatch(QWidget *display,QWidget *native,QEvent *event,const Map &map,const Align &align) {
        const QPointer<QWidget> target=native;
        if(!native || (event->type()==QEvent::DragEnter && (!native->isEnabled() || !native->acceptDrops()))){event->ignore();return;}
        if(event->type()==QEvent::DragLeave){QDragLeaveEvent copy;deliver(native,copy,event);return;}
        auto original=static_cast<QDropEvent *>(event);const auto point=map(original->posF());
        align(point,display->mapToGlobal(original->pos()));
        if(!target){original->ignore();return;}
        if(event->type()==QEvent::DragEnter){QDragEnterEvent copy(point.toPoint(),original->possibleActions(),original->mimeData(),original->mouseButtons(),original->keyboardModifiers());drop(native,copy,original);}
        else if(event->type()==QEvent::DragMove){QDragMoveEvent copy(point.toPoint(),original->possibleActions(),original->mimeData(),original->mouseButtons(),original->keyboardModifiers());drop(native,copy,original);}
        else {QDropEvent copy(point,original->possibleActions(),original->mimeData(),original->mouseButtons(),original->keyboardModifiers());drop(native,copy,original);}
    }
    static QString phase(QEvent::Type type) {
        switch(type){case QEvent::DragEnter:return "drag-enter";case QEvent::DragMove:return "drag-move";
        case QEvent::DragLeave:return "drag-leave";default:return "drop";}
    }
private:
    // Form a base-class member pointer inside a derived access context. Calling
    // it on the real base pointer performs ordinary virtual dispatch, without
    // downcasting the native object or depending on private Qt layouts/symbols.
    struct ViewportAccess : QAbstractScrollArea {
        using Handler=bool(QAbstractScrollArea::*)(QEvent *);
        static Handler handler(){return &ViewportAccess::viewportEvent;}
    };
    static bool deliver(QWidget *native,QEvent &mapped,QEvent *original) {
        auto view=qobject_cast<QAbstractScrollArea *>(native?native->parentWidget():nullptr);
        if(!view || view->viewport()!=native){original->ignore();return false;}
        const bool handled=(view->*ViewportAccess::handler())(&mapped);
        original->setAccepted(mapped.isAccepted());return handled;
    }
    static void drop(QWidget *native,QDropEvent &mapped,QDropEvent *original) {
        mapped.setDropAction(original->dropAction());
        const auto type=mapped.type();const QPointer<QWidget> alive=native;
        const bool sourcePresent=original->source()!=nullptr;
        const int formats=original->mimeData()?original->mimeData()->formats().size():0;
        const bool handled=deliver(native,mapped,original);original->setDropAction(mapped.dropAction());
        if(type!=QEvent::DragMove || !mapped.isAccepted())logEvent("timeline.drop.dispatched",{
            {"phase",phase(type)},{"native_handler_returned",handled},{"accepted",mapped.isAccepted()},
            {"drop_action",int(mapped.dropAction())},{"native_viewport",QString::number(quintptr(alive.data()),16)},
            {"source_present",sourcePresent},{"mime_format_count",formats},
            {"meaning","native handler delivery; not model insertion or display scanout proof"}});
    }
};
}
