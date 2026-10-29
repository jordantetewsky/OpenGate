// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
namespace Unbound {
inline QString nativePresetDirectory() { return QDir::homePath()+"/Library/Preferences/Blackmagic Design/DaVinci Resolve"; }
inline QString nativeWorkflowDirectory() { return "/Library/Application Support/Blackmagic Design/DaVinci Resolve/Workflow Integration Plugins"; }
}
