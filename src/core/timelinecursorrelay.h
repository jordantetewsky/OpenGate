// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QPointer>

namespace Unbound {
// Mirror native hover/grab cursor state, including deferred viewport changes.
// Keep native artwork/hotspots; never invent editing modes or move the OS pointer.
class TimelineCursorRelay : public QObject {
public:
    explicit TimelineCursorRelay(QWidget *display):QObject(display),display(display) {}
    ~TimelineCursorRelay() override { clear(); }
    void track(QWidget *viewport,QPointF point) {
        native=viewport;position=point;active=true;refresh();
    }
    void clear() {
        active=false;
        for(auto widget:watched)if(widget)widget->removeEventFilter(this);
        watched.clear();native=nullptr;
        if(display)display->unsetCursor();
    }
    void refresh() {
        if(!active || !display || !native)return;
        QList<QPointer<QWidget>> next;
        auto cursor=resolve(qobject_cast<QGraphicsView *>(native->parentWidget()),position,next);
        if(auto child=native->childAt(position.toPoint())) {next.append(child);cursor=child->cursor();}
        for(auto widget:watched)if(widget && !next.contains(widget))widget->removeEventFilter(this);
        for(auto widget:next)if(widget && !watched.contains(widget))widget->installEventFilter(this);
        watched=next;display->setCursor(cursor);
    }
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if(event->type()==QEvent::CursorChange)refresh();
        return false;
    }
private:
    static QCursor resolve(QGraphicsView *view,QPointF point,QList<QPointer<QWidget>> &path,int depth=0) {
        if(!view)return QCursor();
        path.append(view);path.append(view->viewport());
        auto cursor=view->viewport()->cursor();
        if(!view->scene() || depth>=8)return cursor;
        auto grabber=view->scene()->mouseGrabberItem();
        const auto items=grabber?QList<QGraphicsItem *>{grabber}:view->items(point.toPoint());
        for(auto item:items) {
            auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
            if(proxy && proxy->widget()) {
                auto widget=proxy->widget();
                const auto at=proxy->mapFromScene(view->mapToScene(point.toPoint()));
                if(auto nested=qobject_cast<QGraphicsView *>(widget))
                    return resolve(nested,at-nested->viewport()->pos(),path,depth+1);
                path.append(widget);
                auto child=widget->childAt(at.toPoint());if(child)path.append(child);
                return child?child->cursor():widget->cursor();
            }
            // Native handlers set the viewport cursor directly for edit-point
            // hit areas. That live choice takes precedence over static items.
            if(view->viewport()->testAttribute(Qt::WA_SetCursor))return cursor;
            for(auto owner=item;owner;owner=owner->parentItem())if(owner->hasCursor())return owner->cursor();
        }
        return cursor;
    }
    QPointer<QWidget> display,native;QPointF position;bool active=false;
    QList<QPointer<QWidget>> watched;
};
}
