// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QSet>
#include "diagnostics.h"
#include "workflowcommandmenu.h"
namespace Unbound {
// Invoke Resolve's own command; never synthesize its key binding or dialog.
inline bool openNativeKeyboardCustomization() {
    QSet<QAction *> matches;
    for (auto widget : QApplication::allWidgets()) if (auto menu=qobject_cast<QMenu *>(widget)) {
        if (menu->property("unboundOwnMenu").toBool()) continue;
        for (auto action : menu->actions()) {
            auto text=action->text(); text.remove('&'); text.remove(QChar(0x2026)); text.remove("...");
            text=text.section('\t',0,0).trimmed();
            if (text == "Keyboard Customization") matches.insert(action);
        }
    }
    if (matches.size()!=1 || !(*matches.begin())->isEnabled()) {
        logEvent("keyboard.launch.unavailable",{{"matching_actions",matches.size()}}); return false;
    }
    auto action=*matches.begin();
    WorkflowCommandMenu::prepareKeyboardEditor();
    logEvent("keyboard.launch.request",{{"action",action->objectName()},{"origin","unbound"}});
    action->trigger(); return true;
}
}
