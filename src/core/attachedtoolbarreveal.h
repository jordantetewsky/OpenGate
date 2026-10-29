// SPDX-License-Identifier: MIT
#pragma once
#include <QLayout>
#include "diagnostics.h"
namespace Unbound {
// Recover an attached toolbar in its existing Timeline. Never reparent it or
// loosen the native-page ownership checks used for actual detachment.
inline bool revealAttachedTimelineToolbar(QWidget *page,QWidget *toolbar,QWidget *timeline) {
    if (!page || !page->isVisible() || !toolbar || !timeline ||
        toolbar->objectName()!="frameEditToolbarTop" || timeline->objectName()!="frameConformTimelines" ||
        !timeline->isAncestorOf(toolbar) || toolbar->isVisible() ||
        !timeline->window()->objectName().startsWith("unboundFloating_")) return false;
    for (auto widget=toolbar;widget && widget!=timeline;widget=widget->parentWidget()) widget->show();
    if (timeline->layout()) timeline->layout()->activate();
    logEvent("toolbar.attached.reveal",{{"toolbar",widgetState(toolbar)},{"owner",widgetState(timeline)},
        {"success",toolbar->isVisible()},{"reparented",false}});
    return toolbar->isVisible();
}
}
