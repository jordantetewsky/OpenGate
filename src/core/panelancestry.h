// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QSet>
namespace Unbound {
// Semantic ancestry across Qt's live embedded widget boundary, without a registry.
inline QWidget *panelParent(QWidget *widget) {
    if(!widget)return nullptr;
    if(auto parent=widget->parentWidget())return parent;
    auto proxy=widget->graphicsProxyWidget();
    if(!proxy || !proxy->scene())return nullptr;
    const auto views=proxy->scene()->views();return views.size()==1?views.front():nullptr;
}
inline QWidget *panelWindow(QWidget *widget) {
    QSet<QWidget *> visited;
    for(auto current=widget;current && !visited.contains(current);current=panelParent(current)) {
        if(current->isWindow() && !current->graphicsProxyWidget())return current;
        visited.insert(current);
    }
    return nullptr;
}
inline bool panelContains(QWidget *owner,QWidget *child) {
    QSet<QWidget *> visited;
    for(auto widget=child;widget && !visited.contains(widget);widget=panelParent(widget)) {
        if(widget==owner)return true;visited.insert(widget);
    }
    return false;
}
}
