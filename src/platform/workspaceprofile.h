// SPDX-License-Identifier: MIT
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "../platforms/windows/workspaceprofile.h"
#elif defined(Q_OS_MACOS)
#include "../platforms/macos/workspaceprofile.h"
#else
#error OpenGate workspace persistence requires a platform directory adapter.
#endif
