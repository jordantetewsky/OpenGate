// SPDX-License-Identifier: MIT
#pragma once
#include "nativeclipcolorhistory.h"
#include "../../core/clipcolorassignmentbackend.h"
#include "../../core/clipcolorstock.h"
#include <QSet>

namespace Unbound {
// Exact-image candidate native document adapter. Capability is explicit; Intel,
// Windows and other Resolve images never enter these private calls.
class MacClipColorAssignmentBackend : public ClipColorAssignmentBackend {
  public:
    using ClipColorAssignmentBackend::ClipColorAssignmentBackend;
    Read read(QGraphicsObject *clip) override {
        try {
            const auto target = MacClipColor::Target::capture(clip);
            if (!target) return {};
            MacClipColor::Record record(*target);
            if (!record.open()) return {};
            const auto value = record.get();
            return {value.valid, value.present, value.bytes, record.nativeColor()};
        } catch (...) { return {}; }
    }
    bool commit(const QVector<Edit> &edits, const QString &label) override {
        if (edits.isEmpty() || edits.size() > 256) return false;
        auto state = std::make_shared<MacClipColor::HistoryState>();
        QSet<QObject *> models;
        try {
            for (const auto &edit : edits) {
                const auto target = MacClipColor::Target::capture(edit.clip);
                if (!target || !edit.before.available || models.contains(target->model.data())) return false;
                models.insert(target->model.data());
                MacClipColor::Record record(*target);
                if (!record.open()) return false;
                const auto before = record.get();
                if (!(before == MacClipColor::Value{true, edit.before.present, edit.before.bytes}) ||
                    record.nativeColor() != edit.before.nativeColor) return false;
                const auto value = nativeEnum(*target, edit.nativeColor);
                if (!value) return false;
                state->changes.append({*target, before, {true, edit.present, edit.bytes}, *value});
            }
            const auto slide = state->changes.front().target.slide;
            MacClipColor::HistoryAction action(slide);
            if (!action.create(state, label)) return false;
            MacClipColor::HistoryCompound scope(slide);
            if (!scope.begin(label)) return false;
            bool good = action.submit(scope.nativeStack()) && !state->failure && state->swaps == 1;
            for (const auto &change : state->changes) {
                if (!good) break;
                good = scope.stock(change.target, change.color) && !state->failure;
            }
            good = good && scope.commit();
            scope.finish();
            return good && !scope.failed && !state->failure;
        } catch (...) { return false; }
    }
  private:
    static std::optional<int> nativeEnum(const MacClipColor::Target &target, QString requested) {
        if (requested.isEmpty()) return -1; // Native Clear, not an inherited RGB copy.
        if (requested.startsWith("ClipColor")) requested.remove(0, 9);
        bool known = false;
        for (const auto &color : clipColorStockReferences()) known |= color.first == requested;
        if (!known || !target.current()) return {};
        // Reuse the native enum->token map rather than guessing alphabetical or
        // menu ordering. Exact-image static ABI evidence: color-history-enum-to-name.
        using Name = QString (*)(int);
        const auto name = reinterpret_cast<Name>(target.slide+0x100520014ULL);
        std::optional<int> found;
        for (int i = 0; i < 16; ++i) {
            auto token = name(i);
            if (token.startsWith("ClipColor")) token.remove(0, 9);
            if (token == requested) {
                if (found) return {};
                found = i;
            }
        }
        return target.current() ? found : std::optional<int>{};
    }
};
} // namespace Unbound
