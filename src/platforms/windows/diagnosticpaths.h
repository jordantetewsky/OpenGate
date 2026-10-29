// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QStandardPaths>
#include <QWidget>
#include <windows.h>
namespace Unbound {
inline QString diagnosticDirectory() {
    const auto local=qEnvironmentVariable("LOCALAPPDATA",QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    return qEnvironmentVariable("UNBOUND_DIAGNOSTICS_DIR",QDir(local).filePath("OpenGate/Logs"));
}
inline QString diagnosticModule() { return QStringLiteral("Unbound.dll"); }
// Native crash-report discovery is separate from OpenGate's session journal.
inline QString diagnosticResolveCrashArchive() { return qEnvironmentVariable("UNBOUND_RESOLVE_CRASH_ARCHIVE"); }
inline qint64 diagnosticNativeWindowNumber(QWidget *widget) {
    return widget && widget->testAttribute(Qt::WA_WState_Created) ? qint64(widget->effectiveWinId()) : 0;
}
inline qint64 diagnosticNativeKeyWindowNumber() { return qint64(reinterpret_cast<quintptr>(GetForegroundWindow())); }
// Qt 5.15.2's Windows mouse handler uses QWindowSystemInterface's elapsed
// process clock, not the Cocoa uptime timestamps used on Mac. Do not compare
// those values to GetTickCount and manufacture an input-age measurement.
inline double diagnosticInputAgeMilliseconds(quint64) { return -1; }
}
