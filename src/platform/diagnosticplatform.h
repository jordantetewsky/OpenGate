// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "../platforms/windows/diagnosticpaths.h"
#else
#include "../platforms/macos/diagnosticpaths.h"
#endif
