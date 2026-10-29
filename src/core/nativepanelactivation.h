// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QStackedWidget>
namespace Unbound {
// Explicit native acquisition is not a user panel recall. GUI thread only.
inline thread_local int nativePanelActivationDepth=0;
inline bool currentNativePanelControl(QAbstractButton *button,QWidget *main) {
    if(button->isVisible())return true;
    if(button->isHidden())return false;
    // Hiding the Workspace Toolbar must not disable native reveal actions.
    // Its current stacked page remains authoritative even while the whole band
    // is hidden. A non-current SSM/DSM/EFTM branch is never a fallback.
    bool currentStack=false;
    for(auto child=static_cast<QWidget *>(button);child && child!=main;child=child->parentWidget()) {
        if(auto stack=qobject_cast<QStackedWidget *>(child->parentWidget())) {
            if(stack->currentWidget()!=child)return false;
            currentStack=true;
        }
    }
    return currentStack;
}
}
