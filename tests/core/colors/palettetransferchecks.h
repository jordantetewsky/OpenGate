// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colorpalettetransfer.h"

namespace OpenGateTest {
template <class Check> void paletteTransferChecks(Check check, const QString &directory) {
    using namespace Unbound;
    const auto path = directory + "/transfer";
    ColorPalette source(path + "/source.json");
    ColorPreset saved;
    saved.id = "transfer-preset";
    saved.name = "Saved VFX";
    saved.category = "custom";
    saved.stops = {{"a", Qt::red, 0, 1}, {"b", Qt::blue, 1, 1}};
    check(source.update(saved).isEmpty(), "transfer fixture saves its original preset");
    auto edited = saved;
    edited.name = "Pending VFX";
    edited.texture = "crosshatch-coarse";
    edited.stops.insert(1, {"middle", Qt::cyan, .4, .8});
    ColorPaletteDrafts original(nullptr, path + "/source-drafts.json");
    original.record(edited, saved);
    QByteArray bytes;
    check(ColorPaletteTransfer::encode(source, original, bytes).isEmpty() &&
              QJsonDocument::fromJson(bytes).object().contains("draftPresets"),
          "explicit export includes saved definitions and separate pending edits");
    ColorPalette destination(path + "/destination.json");
    ColorPaletteDrafts drafts(nullptr, path + "/destination-drafts.json");
    check(ColorPaletteTransfer::apply(bytes, destination, drafts).isEmpty() &&
              destination.find(saved.id) && *destination.find(saved.id) == saved &&
              drafts.get(saved) == edited,
          "builder transfer retains the committed color and editable draft separately");
    {
        ColorPalette reopened(path + "/destination.json");
        ColorPaletteDrafts recovered(nullptr, path + "/destination-drafts.json");
        check(reopened.find(saved.id) && *reopened.find(saved.id) == saved &&
                  recovered.get(saved) == edited,
              "transferred draft survives reopening without silently updating its preset");
    }
    auto local = edited;
    local.name = "Local pending edits";
    drafts.record(local, saved);
    const auto before = ColorPalette::encode(destination.state());
    check(!ColorPaletteTransfer::apply(bytes, destination, drafts).isEmpty() &&
              before == ColorPalette::encode(destination.state()) && drafts.get(saved) == local,
          "a conflicting import cannot overwrite local pending edits or committed colors");
    auto malformed = QJsonDocument::fromJson(bytes).object();
    auto bad = edited.json();
    bad["id"] = "unknown-draft";
    malformed["draftPresets"] = QJsonArray{bad};
    check(!ColorPaletteTransfer::apply(QJsonDocument(malformed).toJson(), destination, drafts)
                  .isEmpty() &&
              before == ColorPalette::encode(destination.state()),
          "drafts without a matching saved color are rejected before any palette write");
    bad = edited.json();
    bad["opacity"] = 101;
    malformed["draftPresets"] = QJsonArray{bad};
    check(!ColorPaletteTransfer::apply(QJsonDocument(malformed).toJson(), destination, drafts)
               .isEmpty(),
          "invalid draft paint is rejected by the shared preset schema");
    QFile blocker(path + "/blocked");
    blocker.open(QIODevice::WriteOnly);
    blocker.write("file");
    blocker.close();
    ColorPalette untouched(path + "/untouched.json");
    ColorPaletteDrafts unwritable(nullptr, path + "/blocked/drafts.json");
    check(!ColorPaletteTransfer::apply(bytes, untouched, unwritable).isEmpty() &&
              !untouched.find(saved.id),
          "draft backup failure prevents committing a palette that would lose pending edits");
    ColorPalette unwritablePalette(path + "/blocked/palette.json");
    const auto recoveryPath = path + "/recoverable-drafts.json";
    ColorPaletteDrafts recoverable(nullptr, recoveryPath);
    const auto failure = ColorPaletteTransfer::apply(bytes, unwritablePalette, recoverable);
    ColorPaletteDrafts reopenedDrafts(nullptr, recoveryPath);
    check(failure.startsWith("Pending edits were backed up") && !unwritablePalette.find(saved.id) &&
              reopenedDrafts.get(saved) == edited,
          "palette write failure retains the new draft backup and reports partial persistence");
    auto unfinished = local;
    unfinished.name.clear();
    drafts.record(unfinished, saved);
    QByteArray unchanged("previous export");
    check(!ColorPaletteTransfer::encode(destination, drafts, unchanged).isEmpty() &&
              unchanged == "previous export",
          "unfinished draft export reports validation without replacing output");
    drafts.record(local, saved);
}
} // namespace OpenGateTest
