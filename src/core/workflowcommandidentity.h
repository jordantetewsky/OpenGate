// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>

namespace Unbound {
// Shared ownership policy for Resolve's existing OpenGate workflow actions.
inline bool nativeOpenGateWorkflowAction(QObject *object)
{
    const auto action = qobject_cast<QAction *>(object);
    return action && action->objectName().startsWith("workspaceWIPlugins_") &&
           action->text().startsWith("OpenGate - ");
}
}
