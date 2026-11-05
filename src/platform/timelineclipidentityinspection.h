// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QObject>
#if defined(Q_OS_MAC) && defined(__arm64__)
#include "../platforms/macos/timelineclipidentityprobe.h"
#endif

namespace Unbound {
// Platform boundary for the opt-in, bounded native clip identity diagnostic.
inline QJsonObject inspectTimelineClipIdentity(QObject *object, bool explicitlyRequested = false) {
    if (!explicitlyRequested && qEnvironmentVariableIntValue("OPENGATE_CLIP_IDENTITY_PROBE") != 1)
        return {{"enabled", false}, {"status", "disabled"}, {"read_only", true}};
#if defined(Q_OS_MAC) && defined(__arm64__)
    return MacTimelineClipIdentity::inspect(object);
#else
    Q_UNUSED(object)
    return {{"enabled", true}, {"status", "unsupported-platform-or-architecture"},
            {"read_only", true}, {"persistent_identity_verified", false}};
#endif
}
} // namespace Unbound
