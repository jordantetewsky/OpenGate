// SPDX-License-Identifier: MIT
#pragma once
#include "nativeclipcolorrecord.h"
#include "../../core/nativecommandactivity.h"
#include <atomic>
#include <memory>

namespace Unbound::MacClipColor {
struct UiActUpdates;
struct Change { Target target; Value expected, next; int color = -1; };
// Native history owns this callable and its before/after records. There is no
// plugin Undo stack, view pointer, menu request TTL or project-title predicate.
// The native void-callback failure limitation remains an explicit release gate.
struct HistoryState {
    QVector<Change> changes;
    std::atomic<int> failure{0}, swaps{0};
    void swap(UiActUpdates &updates) noexcept {
        if (failure) return;
        try {
            for (const auto &change : changes) {
                Record record(change.target);
                if (!record.open() || !(record.get() == change.expected)) { failure = 1; return; }
            }
            int applied = 0;
            for (auto &change : changes) {
                Record record(change.target);
                if (!record.open() || !record.assign(change.next, change.expected)) {
                    // Best-effort explicit rollback is observable as failure;
                    // native callback success is never claimed as rollback proof.
                    failure = 2;
                    for (int i = applied-1; i >= 0; --i) {
                        Record previous(changes[i].target);
                        if (!previous.open() || !previous.assign(changes[i].expected, changes[i].next)) failure = 3;
                    }
                    return;
                }
                ++applied;
            }
            for (auto &change : changes) {
                const auto weak = change.target.weak;
                using Update = void (*)(UiActUpdates *, const void *, quint32, quint32);
                reinterpret_cast<Update>(change.target.slide+0x102bedc6cULL)(&updates, &weak, 0x04100000U, 0);
                reinterpret_cast<void (*)(QObject *)>(change.target.slide+0x10043d4d4ULL)(change.target.model.data());
                std::swap(change.expected, change.next);
            }
            ++swaps;
            if (auto activity = NativeCommandActivity::existing()) activity->completed("clip-color.native-history");
        } catch (...) { failure = 4; }
    }
};

// Own a genuine UiCustomAction through the native shared-handle protocol. This
// is the retained constructor/adoption path, not a manufactured native vtable.
class HistoryAction {
  public:
    explicit HistoryAction(std::uintptr_t slide) : slide(slide) {}
    ~HistoryAction() {
        if (control) reinterpret_cast<void (*)(void **)>(slide+0x1002adcb8ULL)(&control);
    }
    bool create(const std::shared_ptr<HistoryState> &state, const QString &label) {
        using Function = std::function<void(UiActUpdates &)>;
        static_assert(sizeof(Function) == 32, "Exact-image libc++ callable ABI required");
        if (control || state->changes.isEmpty()) return false;
        Function callback = [state](UiActUpdates &updates) noexcept { state->swap(updates); };
        const QString internal = "OpenGateClipPaint";
        using Allocate = void *(*)(std::size_t);
        void *object = reinterpret_cast<Allocate>(slide+0x108542470ULL)(0x58);
        if (!object) return false;
        using Constructor = void (*)(void *, const QString *, const QString *, const Function *);
        bool constructed = false;
        try {
            reinterpret_cast<Constructor>(slide+0x102d9ea5cULL)(object, &internal, &label, &callback);
            constructed = true;
            using MacTimelineClipIdentity::copyMemory;
            std::uintptr_t vtable = 0, candidate = 0, owner = 0;
            std::uint64_t count = 0;
            if (copyMemory(std::uintptr_t(object), &vtable, 8) && vtable == slide+0x11d3d5458ULL &&
                copyMemory(std::uintptr_t(object)+0x18, &candidate, 8) && candidate &&
                copyMemory(candidate, &owner, 8) && owner == std::uintptr_t(object) &&
                copyMemory(candidate+0x10, &count, 8) && count == 1) {
                using Retain = std::uint64_t (*)(void *, std::int64_t);
                reinterpret_cast<Retain>(slide+0x100058bc8ULL)(reinterpret_cast<void *>(candidate+0x10), 0x100000001LL);
                control = reinterpret_cast<void *>(candidate);
                return copyMemory(candidate+0x10, &count, 8) && count == 0x100000002ULL;
            }
        } catch (...) {
            reinterpret_cast<void (*)(void *)>(slide+(constructed ? 0x102dab100ULL : 0x108542368ULL))(object);
            throw;
        }
        reinterpret_cast<void (*)(void *)>(slide+0x102dab100ULL)(object);
        return false;
    }
    bool submit(void *stack) {
        using Submit = bool (*)(void *, void **);
        return control && stack && reinterpret_cast<Submit>(slide+0x103143d2cULL)(stack, &control);
    }
  private:
    Q_DISABLE_COPY(HistoryAction)
    std::uintptr_t slide;
    void *control = nullptr;
};

// Request-local native compound scope. Each metadata action and fallback change
// belongs to the same host event. Destruction aborts unless commit succeeded.
class HistoryCompound {
  public:
    explicit HistoryCompound(std::uintptr_t slide) : slide(slide) {}
    ~HistoryCompound() { finish(); }
    bool begin(const QString &label) {
        if (begun || !qApp || QThread::currentThread() != qApp->thread()) return false;
        auto candidate = reinterpret_cast<void *(*)()>(slide+0x102efbde8ULL)();
        std::uintptr_t vtable = 0;
        if (std::uintptr_t(candidate) != slide+0x11dd3dd18ULL ||
            !MacTimelineClipIdentity::copyMemory(std::uintptr_t(candidate), &vtable, 8) ||
            vtable != slide+0x11d3d1ae8ULL) return false;
        stack = reinterpret_cast<QObject *>(candidate);
        if (!stack || std::uintptr_t(stack->metaObject()) != slide+0x11d3bc1c0ULL ||
            QByteArray(stack->metaObject()->className()) != "UiUndoStack") return false;
        const QString internal = "OpenGateClipPaint";
        using Constructor = void (*)(void *, void *, const QString *, const QString *, bool);
        reinterpret_cast<Constructor>(slide+0x100d9d190ULL)(storage, stack.data(), &internal, &label, false);
        begun = true;
        std::uintptr_t owner = 0, compound = 0;
        std::memcpy(&owner, storage, 8); std::memcpy(&compound, storage+8, 8);
        return owner == std::uintptr_t(candidate) && compound;
    }
    bool stock(const Target &target, int value) {
        if (!begun || finished || !stack || !target.current() || value < -1 || value > 15) return false;
        const auto weak = target.weak;
        struct Vector { const void *begin, *end, *capacity; } selected{&weak, &weak+1, &weak+1};
        using Command = bool (*)(void *, const void *, int);
        return reinterpret_cast<Command>(slide+0x104b36754ULL)(stack.data(), &selected, value);
    }
    bool commit() {
        if (!begun || finished || !stack) return false;
        reinterpret_cast<void (*)(void *)>(slide+0x103149d64ULL)(storage);
        return storage[16] == 1;
    }
    void finish() noexcept {
        if (!begun || finished) return;
        finished = true;
        try { reinterpret_cast<void (*)(void *)>(slide+0x103149b4cULL)(storage); }
        catch (...) { failed = true; }
    }
    void *nativeStack() const { return begun && !finished ? stack.data() : nullptr; }
    bool failed = false;
  private:
    Q_DISABLE_COPY(HistoryCompound)
    std::uintptr_t slide;
    QPointer<QObject> stack;
    alignas(16) unsigned char storage[24]{};
    bool begun = false, finished = false;
};
} // namespace Unbound::MacClipColor
