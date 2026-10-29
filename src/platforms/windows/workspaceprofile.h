// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QStandardPaths>
namespace Unbound {
inline QString workspaceProfileDirectory() {
    // Same directory as the established Windows PaneSettings. Workspace.json
    // and Arrangements.json retain the accepted managed schemas; Unbound.json
    // remains untouched for the existing backend and downgrade recovery.
    auto base=qEnvironmentVariable("LOCALAPPDATA");
    if(base.isEmpty())base=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir(base).filePath("Unbound");
}
}
