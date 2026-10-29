// SPDX-License-Identifier: MIT
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "../platforms/windows/workspaceuiconventions.h"
#elif defined(Q_OS_MACOS)
#include "../platforms/macos/workspaceuiconventions.h"
#else
#error OpenGate workspace UI requires a platform adapter.
#endif
