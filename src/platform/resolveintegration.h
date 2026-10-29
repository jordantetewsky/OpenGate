// SPDX-License-Identifier: MIT
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "../platforms/windows/resolveintegration.h"
#elif defined(Q_OS_MACOS)
#include "../platforms/macos/resolveintegration.h"
#else
#error OpenGate native Resolve integration requires a platform adapter.
#endif
