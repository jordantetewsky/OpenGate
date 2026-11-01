// SPDX-License-Identifier: MIT
#pragma once
#include <QShortcut>
#include <QAction>
#include <QShortcutEvent>
#include "panelancestry.h"
#include "timelinecontentviewport.h"

namespace Unbound {
// Adapt coordinates around an already-selected native shortcut. Qt retains
// command identity, assignments and context; there is no command/key whitelist.
class TimelineShortcutCoordinates {
public:
    static bool dispatch(QObject *receiver,QShortcutEvent *event) {
        if(event->isAmbiguous() || (!qobject_cast<QShortcut *>(receiver) && !qobject_cast<QAction *>(receiver)))return false;
        for(auto widget=QApplication::focusWidget();widget;widget=panelParent(widget))
            if(auto viewport=dynamic_cast<TimelineContentViewport *>(widget)) {
                const bool handled=viewport->dispatchPointerShortcut(receiver,event);
                if(handled)logEvent("timeline.shortcut.coordinates",{{"key",event->key().toString(QKeySequence::PortableText)},
                    {"native_shortcut_deliveries",1},{"os_pointer_moved",false}});
                return handled;
            }
        return false;
    }
};
}
