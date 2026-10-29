// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QWidget>
namespace Unbound {
struct NativeDockScope {
    QPointer<QWidget> window,owner;
    QList<QPointer<QWidget>> cells;
};
inline QList<NativeDockScope> &nativeDockScopes() { static QList<NativeDockScope> scopes; return scopes; }
inline void unregisterNativeDockSurfaces(QWidget *owner) {
    auto &scopes=nativeDockScopes();
    for (int i=scopes.size()-1;i>=0;--i)
        if (!scopes[i].owner || !scopes[i].window || scopes[i].owner==owner) scopes.removeAt(i);
}
// Register actual leaves only; stack targets are derived from their retained
// layout ancestors. Main-window empty space and screen edges stay non-dockable.
inline bool registerNativeDockSurfaces(QWidget *window,QWidget *owner,const QList<QWidget *> &cells) {
    if (!window || !owner || owner==window || owner->isWindow() || owner->window()!=window || cells.size()>32) return false;
    NativeDockScope scope; scope.window=window; scope.owner=owner;
    for (auto cell : cells) {
        if (!cell || cell==owner || !owner->isAncestorOf(cell) || scope.cells.contains(cell)) return false;
        scope.cells.append(cell);
    }
    unregisterNativeDockSurfaces(owner);
    if (nativeDockScopes().size()>=32) return false;
    nativeDockScopes().append(scope); return true;
}
inline QList<QWidget *> nativeDockSurfaces(QWidget *window) {
    QList<QWidget *> result;
    for (const auto &scope : nativeDockScopes()) if (scope.window==window && scope.owner && scope.owner->isVisible())
        for (auto cell : scope.cells) if (cell && cell->isVisible() && scope.owner->isAncestorOf(cell) && cell->width()>0 && cell->height()>0)
            result.append(cell);
    return result;
}
}
