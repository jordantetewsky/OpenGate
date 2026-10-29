// SPDX-License-Identifier: MIT
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "../platforms/windows/workspacewindowadapter.h"
#elif defined(Q_OS_MACOS)
#include "../platforms/macos/workspacewindowadapter.h"
#else
#error OpenGate workspace window integration requires a platform adapter.
#endif
