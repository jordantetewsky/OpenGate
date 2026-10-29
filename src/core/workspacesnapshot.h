// SPDX-License-Identifier: MIT
#pragma once
#include "workspacesettings.h"
#include <QJsonArray>

namespace Unbound {
// Persistence schema only. Capturing live surfaces and initiating transitions
// belong to their owners; serialization never mutates a Resolve widget.
struct WorkspaceSnapshot {
    QJsonObject panels,nativeArrangement,footer,header;
    QJsonArray groups;
    QRect controls;
    QString selected;
    bool pinControls=false,hideBackground=false,panelHierarchy=true,snapWindows=true,keepOnScreen=true;
    int chrome=0;
    QJsonObject json() const
    {
        return {{"version",1},{"pin_controls",pinControls},{"groups",groups},
            {"native_arrangement",nativeArrangement},{"hide_background",hideBackground},
            {"background_policy",hideBackground ? "manual-hidden" : "automatic"},
            {"native_workspace_mode",hideBackground ? "floating" : "native"},{"footer",footer},{"header",header},
            {"selected_arrangement",selected},{"window_chrome",chrome},{"panel_hierarchy",panelHierarchy},
            {"snap_windows",snapWindows},{"keep_on_screen",keepOnScreen},
            {"controls",WorkspaceSettings::rectangle(controls)},{"panels",panels}};
    }
    static QJsonObject panel(QJsonObject previous,bool desired,bool active,const QRect &geometry,
                             bool hidden,bool pinned)
    {
        if (active) {
            previous=WorkspaceSettings::displayRectangle(geometry,previous);
            previous["hidden"]=hidden; previous["pinned"]=pinned;
        }
        previous["floating"]=desired;
        return previous;
    }
};
}
