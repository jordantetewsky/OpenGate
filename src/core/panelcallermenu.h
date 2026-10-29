// SPDX-License-Identifier: MIT
#pragma once
#include "workspacepanelcallers.h"
#include "diagnostics.h"
namespace Unbound {
// Platform diagnostics only; panel inventory and dispatch live in shared core.
class PanelCallerMenu : public WorkspacePanelCallers {
public:
    explicit PanelCallerMenu(QObject *owner=nullptr):WorkspacePanelCallers(owner) {
        report=[](const QString &event,const QJsonObject &details){logEvent(event,details);};
    }
};
}
