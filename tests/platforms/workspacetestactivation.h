// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
#ifdef Q_OS_WIN
#include <qt_windows.h>
namespace Unbound {
inline void activateNativeWindowForTest(QWidget *widget) {
    widget->show();widget->raise();widget->activateWindow();
    SetForegroundWindow(reinterpret_cast<HWND>(widget->winId()));
}
}
#else
#include "../../src/platforms/macos/nativechrome.h"
#endif
