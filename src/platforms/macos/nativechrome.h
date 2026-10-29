// SPDX-License-Identifier: MIT
#pragma once
#include "../../platform/nativewindowcontract.h"
namespace Unbound {
#ifdef UNBOUND_NATIVE_TESTS
QRect nativeHeaderMenuFrameForTest(QWidget *widget);
void clickNativeHeaderMenuForTest(QWidget *widget);
void postNativeCommandRequestForTest();
qint64 nativeFrontmostProcessForTest();
qint64 nativeWindowLevelForTest(QWidget *widget);
void activateNativeWindowForTest(QWidget *widget);
bool saveNativeTrafficForTest(QWidget *widget, const QString &path);
QList<QRect> nativeTrafficFramesForTest(QWidget *widget);
QRect nativeTitleFrameForTest(QWidget *widget);
QString nativeTitleTextForTest(QWidget *widget);
QString nativeTitleToolTipForTest(QWidget *widget);
void clickNativePinForTest(QWidget *widget);
void postNativeMoveStartForTest(QWidget *widget);
void postNativePointerReleaseForTest(QWidget *window);
#endif
}
