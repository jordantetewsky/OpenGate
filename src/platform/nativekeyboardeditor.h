// SPDX-License-Identifier: MIT
#pragma once
#include <QtGlobal>
#if defined(Q_OS_MAC) && defined(__arm64__)
#include "../platforms/macos/nativekeyboardeditor.h"
#else
#include "../platforms/windows/nativekeyboardeditor.h"
#endif
