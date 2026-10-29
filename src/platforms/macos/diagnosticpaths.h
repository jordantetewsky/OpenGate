// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include "nativechrome.h"
namespace Unbound {
inline QString diagnosticDirectory() {
    return qEnvironmentVariable("UNBOUND_DIAGNOSTICS_DIR", QDir::homePath()+"/Library/Logs/OpenGate");
}
inline QString diagnosticModule() { return QStringLiteral("UnboundWidgetProbe.dylib"); }
inline QString diagnosticResolveCrashArchive() {
    return qEnvironmentVariable("UNBOUND_RESOLVE_CRASH_ARCHIVE",QDir::homePath()+"/Library/Application Support/Blackmagic Design/DaVinci Resolve/crash_archive.txt");
}
inline qint64 diagnosticNativeWindowNumber(QWidget *widget) { return nativeWindowNumber(widget); }
inline qint64 diagnosticNativeKeyWindowNumber() { return nativeKeyWindowNumber(); }
inline double diagnosticInputAgeMilliseconds(quint64 timestamp) { return nativeInputAgeMilliseconds(timestamp); }
}
