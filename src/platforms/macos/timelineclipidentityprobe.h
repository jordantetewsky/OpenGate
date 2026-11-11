// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QJsonObject>
#include <QPointer>
#include <QThread>
#include "nativeclipidentitysnapshot.h"
#include "nativeimageverification.h"
#include "nativeclipmetadatainspection.h"

namespace Unbound::MacTimelineClipIdentity {
// Keep the clip adapter's existing interface while sharing exact-image checks
// with the keyboard adapter. Assignment/history callers retain the same gate.
using MacNativeImageVerification::copyMemory;
using Image=MacNativeImageVerification::Image;
inline Image verifyImage() {
    return MacNativeImageVerification::verifyImage(NativeClipIdentity::imageHash,NativeClipIdentity::imageSize);
}
inline const Image &verifiedImage() {
    static const Image image=verifyImage();
    return image;
}
inline bool isSupportedUiObject(QObject *object) {
    return object && object->inherits("UiTiItem") &&
           NativeClipIdentity::isSupportedUiClassName(object->metaObject()->className());
}
inline QJsonObject inspect(QObject *object) {
    QJsonObject result{{"enabled", true}, {"read_only", true}, {"private_calls", false},
                       {"sdk_equality_verified", false}, {"persistent_identity_verified", false}};
    if (!qApp || QThread::currentThread() != qApp->thread()) {
        result["status"] = "requires-gui-thread";
        return result;
    }
    QPointer<QObject> alive = object;
    if (!alive || !isSupportedUiObject(object)) {
        result["status"] = "unsupported-ui-class";
        return result;
    }
    const auto &image = verifiedImage(); // Only an explicit opt-in inspection reaches this.
    result["image_verified"] = image.valid;
    result["image_status"] = image.status;
    result["supported_arm64_sha256"] = NativeClipIdentity::imageHash;
    if (!image.valid) {
        result["status"] = image.status;
        return result;
    }
    int reads = 0;
    const NativeClipIdentity::Reader reader = [&reads](std::uintptr_t p, void *out, std::size_t n) {
        return ++reads <= 64 && n <= 256 && MacNativeImageVerification::copyMemory(p, out, n);
    };
    const auto snapshot = NativeClipIdentity::inspect(reader, std::uintptr_t(object), image.slide,
                                                     [&alive] { return !alive.isNull(); });
    result["status"] = snapshot.status;
    result["identity_chain_valid"] = snapshot.valid;
    result["bounded_memory_reads"] = reads;
    if (snapshot.valid) {
        result["native_timeline_item_id"] = QString::fromStdString(snapshot.id);
        // The getter is never called without an externally prepared disposable
        // SDK-ID allowlist; the normal identity inspector performs memory reads.
        if (qEnvironmentVariableIntValue("OPENGATE_CLIP_IDENTITY_READ_METADATA") == 1) {
            const auto metadata = inspectDisposableClipMetadata(
                reinterpret_cast<QObject *>(snapshot.model), QString::fromStdString(snapshot.id));
            const auto after = NativeClipIdentity::inspect(
                reader, std::uintptr_t(object), image.slide,
                [&alive] { return !alive.isNull(); });
            if (after.valid && after.id == snapshot.id && after.model == snapshot.model)
                result["metadata"] = metadata;
            else {
                result.remove("native_timeline_item_id");
                result["identity_chain_valid"] = false;
                result["status"] = "identity-changed-during-metadata-read";
            }
        }
    }
    result["bounded_memory_reads"] = reads;
    return result;
}
} // namespace Unbound::MacTimelineClipIdentity
