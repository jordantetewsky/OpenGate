// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
#include <QVariant>
#include <QList>
#include <QSet>
#include <algorithm>
#include "droppolicy.h"
namespace Unbound {
inline QString dockSectionKey(QWidget *surface) {return surface?surface->property("openGateDockSection").toString():QString();}
inline QString dockTargetKey(QWidget *surface) {
    const auto section=dockSectionKey(surface);
    return !section.isEmpty()?section:surface?surface->property("groupPanel").toString():QString();
}
struct DockScopeHit { QWidget *surface=nullptr; QRect frame; DropSelection drop; };
// Leaf edges keep their own bounds. Edge-center handles select a whole retained
// split/tab section; nested handles are inset to remain independently reachable.
inline DockScopeHit selectDockScope(const QList<QWidget *> &leaves,QWidget *window,QPoint pointer) {
    QList<QWidget *> sections;QSet<QWidget *> seen;DockScopeHit leaf;
    for(auto surface:leaves) {
        if(!surface || !surface->isVisible())continue;
        auto cell=surface->property("groupPanel").isValid()?surface:surface->parentWidget();
        if(!cell)continue;
        const QRect frame(cell->mapToGlobal(QPoint()),cell->size());
        if(frame.contains(pointer) && (!leaf.surface || qint64(frame.width())*frame.height()<qint64(leaf.frame.width())*leaf.frame.height()))leaf={surface,frame,{}};
        for(auto parent=cell->parentWidget();parent && parent!=window;parent=parent->parentWidget())
            if(!dockSectionKey(parent).isEmpty() && parent->isVisible() && !seen.contains(parent)){seen.insert(parent);sections.append(parent);}
    }
    auto depth=[window](QWidget *node){int n=0;for(auto p=node->parentWidget();p&&p!=window;p=p->parentWidget())n+=!dockSectionKey(p).isEmpty();return n;};
    std::sort(sections.begin(),sections.end(),[&](QWidget *a,QWidget *b){return depth(a)<depth(b);});
    bool neutral=false;
    for(auto section:sections) {
        const QRect frame(section->mapToGlobal(QPoint()),section->size());if(!frame.contains(pointer))continue;
        const int inset=depth(section)*30;
        const int x=qMin(inset,qMax(0,frame.width()/3-24)),y=qMin(inset,qMax(0,frame.height()/3-24));
        const int wide=qMax(8,qMin(72,frame.width()/3)),high=qMax(8,qMin(72,frame.height()/3));
        const QList<QRect> handles{
            QRect(frame.left()+x,frame.center().y()-high/2,qMin(24,frame.width()),high),
            QRect(frame.right()-x-qMin(24,frame.width())+1,frame.center().y()-high/2,qMin(24,frame.width()),high),
            QRect(frame.center().x()-wide/2,frame.top()+y,wide,qMin(24,frame.height())),
            QRect(frame.center().x()-wide/2,frame.bottom()-y-qMin(24,frame.height())+1,wide,qMin(24,frame.height()))};
        const QList<QPoint> edges{{frame.left(),frame.center().y()},{frame.right(),frame.center().y()},{frame.center().x(),frame.top()},{frame.center().x(),frame.bottom()}};
        for(int i=0;i<handles.size();++i) {
            if(handles[i].contains(pointer)){auto drop=selectPanelDrop(frame,edges[i]);drop.region=handles[i];return {section,frame,drop};}
            neutral|=handles[i].adjusted(-4,-4,4,4).contains(pointer);
        }
    }
    return neutral?DockScopeHit{}:leaf;
}
}
