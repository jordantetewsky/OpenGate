// SPDX-License-Identifier: MIT
#pragma once
#include "timelineclipidentityprobe.h"
#include <QGraphicsObject>
#include <QMetaType>
#include <QPointer>
#include <optional>

namespace Unbound::MacClipColor {
inline constexpr char assignmentKey[] = "com.cinewaltz.opengate.clip-paint.v1";
struct Value { bool valid = false, present = false; QByteArray bytes; };
inline bool operator==(const Value &a, const Value &b) {
    return a.valid && b.valid && a.present == b.present && a.bytes == b.bytes;
}
// Captured model identity is independent of the originating view and SDK proxy.
// QPointer is checked before reading its native weak handle on the GUI thread.
struct Target {
    QPointer<QObject> model;
    QString id;
    std::uintptr_t weak = 0, slide = 0;
    bool current() const {
        if (!qApp || QThread::currentThread() != qApp->thread() || !model) return false;
        using NativeClipIdentity::read;
        const NativeClipIdentity::Reader reader = MacTimelineClipIdentity::copyMemory;
        std::uintptr_t owner = 0, data = 0;
        quint8 flags = 1;
        qint32 count = 0;
        qint64 offset = 0;
        if (!read(reader, weak, 0, owner) || owner != std::uintptr_t(model.data()) ||
            !read(reader, weak, 0x20, flags) || (flags & 1) ||
            !read(reader, owner, 0x18, data) || !read(reader, data, 4, count) ||
            !read(reader, data, 16, offset) || count < 1 || count > 128 || offset < 24 || offset > 4096)
            return false;
        char16_t characters[128]{};
        if (std::uintptr_t(offset) > std::numeric_limits<std::uintptr_t>::max()-data) return false;
        return reader(data + std::uintptr_t(offset), characters, std::size_t(count)*2) &&
            QString::fromUtf16(reinterpret_cast<const ushort *>(characters), count) == id;
    }
    static std::optional<Target> capture(QGraphicsObject *container) {
        if (!container || !qApp || QThread::currentThread() != qApp->thread()) return {};
        const auto &image = MacTimelineClipIdentity::verifiedImage();
        if (!image.valid) return {};
        auto pending = container->childItems();
        std::optional<Target> found;
        int visits = 0;
        while (!pending.isEmpty() && ++visits <= 64) {
            auto item = pending.takeFirst();
            auto object = dynamic_cast<QObject *>(item);
            if (object && MacTimelineClipIdentity::isSupportedUiObject(object)) {
                QPointer<QObject> alive = object;
                int reads = 0;
                const NativeClipIdentity::Reader reader = [&reads](auto p, auto out, auto n) {
                    return ++reads <= 64 && n <= 256 && MacTimelineClipIdentity::copyMemory(p, out, n);
                };
                const auto snapshot = NativeClipIdentity::inspect(reader, std::uintptr_t(object),
                    image.slide, [&alive] { return bool(alive); }, false);
                if (!snapshot.valid) return {};
                Target next;
                next.model = reinterpret_cast<QObject *>(snapshot.model);
                next.id = QString::fromStdString(snapshot.id);
                next.slide = image.slide;
                if (!MacTimelineClipIdentity::copyMemory(std::uintptr_t(object)+0xc0, &next.weak, 8) ||
                    !next.current() || (found && found->model != next.model)) return {};
                found = next;
            }
            for (auto child : item->childItems()) {
                if (pending.size() >= 64) return {};
                pending.append(child);
            }
        }
        return pending.isEmpty() ? found : std::optional<Target>{};
    }
};

// One bounded native BtString temporary. Allocation/constructor/destructor ABI
// comes from the retained exact-image canonical carrier, not a Qt shadow field.
class String {
  public:
    explicit String(std::uintptr_t slide) : slide(slide) {}
    bool initialize(const QByteArray &bytes) {
        if (initialized) return false;
        using Constructor = void *(*)(void *, const char *, int, bool);
        const auto result = reinterpret_cast<Constructor>(slide+0x100158a38ULL)(storage, bytes.constData(), bytes.size(), false);
        initialized = true;
        return result == storage;
    }
    ~String() { if (initialized) reinterpret_cast<void (*)(void *)>(slide+0x100158c50ULL)(storage); }
    const void *data() const { return storage; }
  private:
    Q_DISABLE_COPY(String)
    std::uintptr_t slide;
    alignas(16) unsigned char storage[56]{};
    bool initialized = false;
};

// Read/compare/write only OpenGate's fixed key. Foreign native metadata stays in
// its existing map and is neither decoded nor replaced by this adapter.
class Record {
  public:
    explicit Record(const Target &target) : target(target) {}
    bool open() {
        if (!target.current() || QMetaType::type("BtString") == QMetaType::UnknownType ||
            QMetaType::sizeOf(QMetaType::type("BtString")) != 56) return false;
        using NativeClipIdentity::read;
        quint8 flags = 1;
        std::uintptr_t vtable = 0;
        if (!read(reader, std::uintptr_t(target.model.data()), 0x440, weak) ||
            !read(reader, weak, 0x20, flags) || (flags & 1) || !read(reader, weak, 0, blob) ||
            !read(reader, blob, 0, vtable) || vtable != target.slide+0x11d2cefb0ULL ||
            !read(reader, blob, 0x258, vtable) || vtable != target.slide+0x11d2ba650ULL) return false;
        metadata = blob+0x258;
        return current();
    }
    QString nativeColor() const {
        if (!current()) return {};
        using NativeClipIdentity::read;
        std::uintptr_t data = 0;
        qint32 length = 0;
        qint64 offset = 0;
        if (!read(reader, blob, 0x3f8, data) || !read(reader, data, 4, length) ||
            !read(reader, data, 16, offset) || length < 0 || length > 64 || offset < 24 || offset > 4096) return {};
        char16_t characters[64]{};
        if (std::uintptr_t(offset) > std::numeric_limits<std::uintptr_t>::max()-data) return {};
        if (length && !reader(data+std::uintptr_t(offset), characters, std::size_t(length)*2)) return {};
        return current() ? QString::fromUtf16(reinterpret_cast<const ushort *>(characters), length) : QString();
    }
    Value get() const {
        if (!current()) return {};
        String key(target.slide);
        if (!key.initialize(assignmentKey) || !current()) return {};
        using Lookup = const void *(*)(void *, const void *);
        const auto borrowed = reinterpret_cast<std::uintptr_t>(reinterpret_cast<Lookup>(target.slide+0x10012649cULL)(
            reinterpret_cast<void *>(metadata), key.data()));
        if (!borrowed || !current()) return {};
        Value value;
        value.present = borrowed != target.slide+0x11d8a4af0ULL;
        using NativeClipIdentity::read;
        std::uintptr_t data = 0, afterData = 0;
        qint32 length = 0, afterLength = 0;
        if (!read(reader, borrowed, 0, data) || !read(reader, borrowed, 0x2c, length) ||
            length < 0 || length > 4096 || (length && !data)) return {};
        value.bytes.resize(length);
        for (int at = 0; at < length; at += 256) {
            if (std::uintptr_t(at) > std::numeric_limits<std::uintptr_t>::max()-data ||
                !reader(data+std::uintptr_t(at), value.bytes.data()+at, std::size_t(qMin(256, length-at)))) return {};
        }
        if (!current() || !read(reader, borrowed, 0, afterData) || afterData != data ||
            !read(reader, borrowed, 0x2c, afterLength) || afterLength != length ||
            (!value.present && length)) return {};
        value.valid = true;
        return value;
    }
    bool assign(const Value &next, const Value &expected) {
        if (!next.valid || next.bytes.size() > 4096 || next.bytes.contains('\0') ||
            (!next.present && !next.bytes.isEmpty()) || !(get() == expected)) return false;
        if (next == expected) return true;
        String key(target.slide);
        if (!key.initialize(assignmentKey) || !current()) return false;
        if (next.present) {
            String bytes(target.slide);
            if (!bytes.initialize(next.bytes) || !current()) return false;
            using Writer = void (*)(void *, const void *, const void *);
            reinterpret_cast<Writer>(target.slide+0x10012658cULL)(reinterpret_cast<void *>(metadata), key.data(), bytes.data());
        } else {
            using MapGetter = void *(*)(void *);
            const auto map = reinterpret_cast<std::uintptr_t>(reinterpret_cast<MapGetter>(target.slide+0x100126394ULL)(reinterpret_cast<void *>(metadata)));
            if (map != metadata+8 || !current()) return false;
            using Erase = std::uint64_t (*)(void *, const void *);
            if (reinterpret_cast<Erase>(target.slide+0x100e73360ULL)(reinterpret_cast<void *>(map+8), key.data()) != 1 || !current()) return false;
            reinterpret_cast<void (*)(void *)>(target.slide+0x1001260bcULL)(reinterpret_cast<void *>(metadata));
        }
        return get() == next;
    }
  private:
    const Target &target;
    std::uintptr_t weak = 0, blob = 0, metadata = 0;
    mutable int reads = 0;
    const NativeClipIdentity::Reader reader = [this](auto p, auto out, auto n) {
        return ++reads <= 384 && n <= 256 && MacTimelineClipIdentity::copyMemory(p, out, n);
    };
    bool current() const {
        using NativeClipIdentity::read;
        std::uintptr_t nowWeak = 0, nowBlob = 0;
        quint8 flags = 1;
        return target.current() && read(reader, std::uintptr_t(target.model.data()), 0x440, nowWeak) &&
            nowWeak == weak && read(reader, weak, 0x20, flags) && !(flags & 1) &&
            read(reader, weak, 0, nowBlob) && nowBlob == blob;
    }
};
} // namespace Unbound::MacClipColor
