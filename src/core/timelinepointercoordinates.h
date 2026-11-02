// SPDX-License-Identifier: MIT
#pragma once
#include <QCursor>
#include <QPointer>
#include <QWidget>
#include <functional>

namespace Unbound {
// Keep native global-coordinate queries at the displayed pointer through queued
// handlers. The native origin is temporary presentation state, never a setting.
class TimelinePointerCoordinates {
public:
    TimelinePointerCoordinates(QWidget *display,QWidget *viewport,std::function<QPointF(QPointF)> mapping)
        :display(display),viewport(viewport),surface(viewport?viewport->parentWidget():nullptr),mapping(std::move(mapping)) {
        if(surface)origin=surface->pos();
    }
    ~TimelinePointerCoordinates(){restore();}
    bool align(QPointF displayed,QPoint physical) {
        return alignNative(mapping(displayed),physical);
    }
    bool alignNative(QPointF point,QPoint physical) {
        if(!display || !viewport || !surface)return false;
        const auto shift=physical-viewport->mapToGlobal(point.toPoint());
        if(!shift.isNull())surface->move(surface->pos()+shift);
        return true;
    }
    bool alignCursor() {
        if(!display)return false;
        const auto physical=QCursor::pos();const auto at=display->mapFromGlobal(physical);
        return display->rect().contains(at) && align(at,physical);
    }
    void geometryChanged() {if(surface){origin=surface->pos();alignCursor();}}
    void restore(){if(surface)surface->move(origin);}
private:
    QPointer<QWidget> display,viewport,surface;QPoint origin;
    std::function<QPointF(QPointF)> mapping;
};
}
