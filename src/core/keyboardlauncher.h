// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QSet>
#include "diagnostics.h"
#include "workflowcommandmenu.h"
namespace Unbound {
// Discover the unique native action; temporary OpenGate menu leases are excluded.
inline QAction *nativeKeyboardCustomizationAction() {
    QSet<QAction *> matches;
    for (auto widget : QApplication::allWidgets()) if (auto menu=qobject_cast<QMenu *>(widget)) {
        if (menu->property("unboundOwnMenu").toBool()) continue;
        for (auto action : menu->actions()) {
            if(action->property("openGateNativeKeyboardEntry").toBool())continue;
            auto text=action->text(); text.remove('&'); text.remove(QChar(0x2026)); text.remove("...");
            text=text.section('\t',0,0).trimmed();
            if (text == "Keyboard Customization") matches.insert(action);
        }
    }
    return matches.size()==1 ? *matches.begin() : nullptr;
}
// Invoke Resolve's own command; never synthesize its key binding or dialog.
inline bool openNativeKeyboardCustomization() {
    auto action=nativeKeyboardCustomizationAction();
    if(!action || !action->isEnabled()) {
        logEvent("keyboard.launch.unavailable",{{"matching_actions",action ? 1 : 0}});return false;
    }
    WorkflowCommandMenu::prepareKeyboardEditor();
    logEvent("keyboard.launch.request",{{"action",action->objectName()},{"origin","unbound"}});
    action->trigger(); return true;
}
}
