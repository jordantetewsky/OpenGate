// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonObject>
#include <QPointer>
#include <QThread>
#include <QtEndian>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <limits>
#include "nativeclipidentitysnapshot.h"
#include "nativeclipmetadatainspection.h"

namespace Unbound::MacTimelineClipIdentity {
// Exact-image validation and bounded current-task reads for an explicit inspector.
inline bool copyMemory(std::uintptr_t address, void *target, std::size_t size) {
    if (!address || !target || !size || size > 65536 ||
        size > std::numeric_limits<std::uintptr_t>::max() - address)
        return false;
    mach_vm_size_t copied = 0;
    return mach_vm_read_overwrite(mach_task_self(), mach_vm_address_t(address), size,
                                 mach_vm_address_t(target), &copied) == KERN_SUCCESS &&
           copied == size;
}
struct Image {
    bool valid = false;
    const char *status = "unsupported-native-image";
    std::uintptr_t slide = 0;
};
inline Image verifyImage() {
    mach_header_64 loaded{};
    const auto header = _dyld_get_image_header(0);
    if (!copyMemory(std::uintptr_t(header), &loaded, sizeof(loaded)) ||
        loaded.magic != MH_MAGIC_64 || loaded.cputype != CPU_TYPE_ARM64)
        return {};
    const auto name = _dyld_get_image_name(0);
    QFile file(name ? QString::fromUtf8(name) : QString());
    if (!file.open(QIODevice::ReadOnly))
        return {false, "native-image-unreadable", 0};
    const auto head = file.read(8);
    if (head.size() != 8)
        return {};
    const auto magic = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(head.constData()));
    quint64 offset = 0, size = quint64(file.size());
    if (magic == 0xcafebabeU || magic == 0xcafebabfU) {
        const auto count = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(head.constData()+4));
        if (!count || count > 64)
            return {};
        const int entrySize = magic == 0xcafebabfU ? 32 : 20;
        const auto entries = file.read(qint64(count) * entrySize);
        if (entries.size() != int(count) * entrySize)
            return {};
        bool found = false;
        for (quint32 i = 0; i < count; ++i) {
            const auto p = reinterpret_cast<const uchar *>(entries.constData()+i*entrySize);
            if (qFromBigEndian<quint32>(p) != quint32(CPU_TYPE_ARM64))
                continue;
            if (found)
                return {};
            found = true;
            offset = entrySize == 32 ? qFromBigEndian<quint64>(p+8) : qFromBigEndian<quint32>(p+8);
            size = entrySize == 32 ? qFromBigEndian<quint64>(p+16) : qFromBigEndian<quint32>(p+12);
        }
        if (!found)
            return {};
    }
    if (size != NativeClipIdentity::imageSize || offset > quint64(file.size()) ||
        size > quint64(file.size()) - offset || !file.seek(qint64(offset)))
        return {};
    // Confirm the loaded image's UUID/header against the selected on-disk slice.
    mach_header_64 disk{};
    if (file.read(reinterpret_cast<char *>(&disk), sizeof(disk)) != qint64(sizeof(disk)) ||
        disk.magic != loaded.magic || disk.cputype != loaded.cputype ||
        disk.cpusubtype != loaded.cpusubtype || disk.sizeofcmds != loaded.sizeofcmds ||
        disk.ncmds != loaded.ncmds || loaded.sizeofcmds > 65536)
        return {};
    QByteArray commands(int(loaded.sizeofcmds), Qt::Uninitialized);
    if (!copyMemory(std::uintptr_t(header)+sizeof(loaded), commands.data(), std::size_t(commands.size())) ||
        commands != file.read(loaded.sizeofcmds))
        return {false, "loaded-native-image-mismatch", 0};
    if (!file.seek(qint64(offset)))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    quint64 remaining = size;
    while (remaining) {
        const auto bytes = file.read(qint64(qMin<quint64>(remaining, 1024*1024)));
        if (bytes.isEmpty())
            return {false, "native-image-unreadable", 0};
        hash.addData(bytes);
        remaining -= quint64(bytes.size());
    }
    if (hash.result().toHex() != NativeClipIdentity::imageHash)
        return {};
    return {true, "exact-image-verified", std::uintptr_t(_dyld_get_image_vmaddr_slide(0))};
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
    static const Image image = verifyImage(); // Only an explicit opt-in inspection reaches this.
    result["image_verified"] = image.valid;
    result["image_status"] = image.status;
    result["supported_arm64_sha256"] = NativeClipIdentity::imageHash;
    if (!image.valid) {
        result["status"] = image.status;
        return result;
    }
    int reads = 0;
    const NativeClipIdentity::Reader reader = [&reads](std::uintptr_t p, void *out, std::size_t n) {
        return ++reads <= 64 && n <= 256 && copyMemory(p, out, n);
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
