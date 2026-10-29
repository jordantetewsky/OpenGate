// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
namespace Unbound {
// Shared product policy. Platform adapters supply the native panel recipe;
// no window discovery, native APIs or presentation ownership belongs here.
struct WorkspacePresetPolicy {
    static QString resolveLayoutId(const QString &page) { return QStringLiteral("opengate.resolve-layout.")+page; }
    static bool builtIn(const QString &id) { return id.startsWith(QStringLiteral("opengate.resolve-layout.")); }
    static bool writable(const QString &id) { return !id.isEmpty() && !builtIn(id); }
};
}
