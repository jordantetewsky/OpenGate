// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
namespace Unbound {
inline int workspaceWindowTier(QWidget *window) {
    if (!window) return 0;
    if (window->property("unboundPinned").toBool()) return 5;
    // Group names describe their creation, not their current membership.
    // A Timeline workspace remains below utility windows after merges/tear-offs.
    if (window->objectName()=="unboundFloating_frameConformTimelines" ||
        window->findChild<QWidget *>("frameConformTimelines")) return 0;
    if (window->objectName()=="unboundFloating_widgetEditInspector" ||
        window->findChild<QWidget *>("widgetEditInspector")) return 3;
    if (window->objectName()=="unboundFloating_widgetEditSourcePlayer" ||
        window->objectName()=="unboundFloating_widgetEditTimelinePlayer") return 2;
    return 1;
}
}
