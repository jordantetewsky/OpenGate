// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QHash>
#include "../platform/workspacewindowintegration.h"
#include "docksurfaces.h"
#include "nativedocksurfaces.h"
#include "dockscopetargets.h"
namespace Unbound {
struct DockDestination { QWidget *surface = nullptr; QRect frame; DropSelection drop; };
inline DockDestination resolveDockDestination(QWidget *source, QPoint pointer, const QList<QWidget *> &overlays)
{
    QWidget *destination = nullptr;
    QHash<qint64, QWidget *> windows;
    QList<QWidget *> underPointer; bool possible = false;
    for (auto candidate : QApplication::topLevelWidgets()) {
        if (candidate == source || overlays.contains(candidate) || !candidate->isVisible() || candidate->isMinimized()) continue;
        if (!candidate->frameGeometry().contains(pointer)) continue;
        underPointer.append(candidate); possible |= candidate->objectName().startsWith("unboundFloating_") || !nativeDockSurfaces(candidate).isEmpty();
    }
    // Free dragging needs no WindowServer query. When a target is possible we
    // still use the current native stack, including non-dockable occluders.
    if (!possible) return {};
    for (auto candidate : underPointer) windows.insert(nativeWindowNumber(candidate),candidate);
    for (auto number : nativeOrderedWindowNumbers()) {
        auto hit = windows.value(number);
        if (!hit || !hit->frameGeometry().contains(pointer)) continue;
        if (hit->objectName().startsWith("unboundFloating_") || !nativeDockSurfaces(hit).isEmpty()) destination = hit;
        break;
    }
    if (!destination) return {};
    const auto surfaces=destination->objectName().startsWith("unboundFloating_")?dockSurfaces(destination):nativeDockSurfaces(destination);
    if(surfaces.isEmpty())return destination->objectName().startsWith("unboundFloating_")?DockDestination{destination,destination->frameGeometry(),{}}:DockDestination{};
    const auto hit=selectDockScope(surfaces,destination,pointer);
    return {hit.surface,hit.frame,hit.drop};
}
}
