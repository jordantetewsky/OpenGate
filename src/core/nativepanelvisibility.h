// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QScopedValueRollback>
#include "diagnostics.h"
#include "nativepanelactivation.h"
namespace Unbound {
inline bool setNativePanelVisibility(QWidget *main,const QString &toggle,bool visible,QWidget *target=nullptr) {
    if (!main || toggle.isEmpty()) return false;
    const auto base="btnEditToggle"+toggle;
    for (auto button:main->findChildren<QAbstractButton *>()) {
        const auto name=button->objectName();
        if (name!=base+"InSSM" && name!=base+"InDSM" && name!=base+"InEFTM") continue;
        // Inactive SSM/DSM/EFTM variants retain checked states.
        if (!currentNativePanelControl(button,main) || !button->isEnabled() || !button->isCheckable()) continue;
        const bool stale=target && target->isVisible()!=visible && button->isChecked()==visible;
        if(stale) {
            // OpenGate mirrors caller state with blocked signals while content
            // is leased. On native reacquisition, that mirror is not evidence
            // that Resolve has revealed the original owner. Dispatch exactly
            // one normal native activation, with the requested checked value.
            const QSignalBlocker block(button);button->setChecked(!visible);
        }
        if (button->isChecked()!=visible) {
            const QScopedValueRollback<int> nativeActivation(nativePanelActivationDepth,nativePanelActivationDepth+1);
            button->click();
        }
        logEvent("panel.visibility.native-control",{{"toggle",toggle},{"button",name},{"requested",visible},{"checked",button->isChecked()},{"reconciled_stale_check",stale},{"target_visible",target && target->isVisible()},{"control_visible",button->isVisible()}});
        return button->isChecked()==visible;
    }
    return false;
}
}
