// SPDX-License-Identifier: MIT
#pragma once
#include "../core/clipcolorassignmentbackend.h"
#include <QApplication>
#include <QPointer>
#if defined(Q_OS_MAC) && defined(__arm64__)
#include "../platforms/macos/clipcolorassignmentbackend.h"
#endif
namespace Unbound {
// An explicitly scoped local test backend can outlive the view it first tests.
// The session borrows it. Its document guard remains authoritative.
inline QPointer<ClipColorAssignmentBackend> &sessionClipColorAssignmentBackend() {
    static QPointer<ClipColorAssignmentBackend> backend;
    return backend;
}
inline ClipColorAssignmentBackend *platformClipColorAssignmentBackend() {
    if (auto backend = sessionClipColorAssignmentBackend()) return backend;
#if defined(Q_OS_MAC) && defined(__arm64__)
    // Local candidate builds can test ordinary startup. Distribution builds
    // retain the explicit opt-in while native failure handling is under review.
    bool configured = false;
    const auto requested = qEnvironmentVariableIntValue("OPENGATE_PERSISTENT_CLIP_COLORS", &configured);
#if defined(OPENGATE_LOCAL_PERSISTENT_CLIP_COLORS)
    if (configured && requested != 1) return nullptr;
#else
    if (!configured || requested != 1) return nullptr;
#endif
    const auto &image = MacTimelineClipIdentity::verifiedImage();
    if (!image.valid) return nullptr;
    static QPointer<ClipColorAssignmentBackend> backend;
    if (!backend && qApp && !QCoreApplication::closingDown())
        backend = new MacClipColorAssignmentBackend(qApp);
    return backend;
#else
    return nullptr;
#endif
}
} // namespace Unbound
