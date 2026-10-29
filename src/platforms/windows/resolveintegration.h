// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QStandardPaths>
namespace Unbound {
inline QString nativePresetDirectory() {
    // Read-only provider; missing files fail closed in NativePresetBindings.
    // The actual Resolve installation/profile remains a Windows live check.
    const auto base=qEnvironmentVariable("APPDATA",QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    return QDir(base).filePath("Blackmagic Design/DaVinci Resolve/Preferences");
}
inline QString nativeWorkflowDirectory() {
    // Resolve's Developer/Workflow Integrations/README.txt, root directory.
    const auto base=qEnvironmentVariable("PROGRAMDATA");
    return base.isEmpty()?QString():QDir(base).filePath("Blackmagic Design/DaVinci Resolve/Support/Workflow Integration Plugins");
}
}
