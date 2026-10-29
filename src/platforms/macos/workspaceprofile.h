// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
namespace Unbound {
inline QString workspaceProfileDirectory() { return QDir::homePath()+"/Library/Application Support/Unbound"; }
}
