// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/clippainttrace.h"

namespace OpenGateTest {
template <class Check> void primitiveEvidenceChecks(Check check) {
    Unbound::ColorPreset preset;
    preset.id = "diagnostic-preset";
    auto style = QSharedPointer<const Unbound::ColorPreset>::create(preset);
    Unbound::ClipFillRegion region{{0, 0, 400, 200}, Qt::cyan, {}, style};
    region.graphicsIdentity = 0x1234;
    QVector<QJsonObject> receipts;
    Unbound::ClipPaintTrace trace(true, [&](const auto &value) { receipts.append(value); });
    trace.begin({region}, {}, 1);
    for (int i = 0; i < 20; ++i)
        trace.observe("solid-fill", {220, 0, 180, 160}, {}, false, QColor("#e8aa00"));
    check(receipts.size() == 1 && receipts[0]["graphics_item"] == "1234" &&
              receipts[0]["kind"] == "solid-fill" && receipts[0]["source_color"] == "#ffe8aa00" &&
              !receipts[0]["mapped"].toBool() && !receipts[0]["pixel_content_captured"].toBool(),
          "native fragment metadata identifies an unmapped primitive without image pixels");
    trace.observe("solid-fill", {218, 0, 2, 200}, {}, false, Qt::red);
    check(receipts.size() == 1, "paint evidence ignores narrow playhead-like primitives");
    trace.observe("pixmap", {220, 0, 180, 160}, {}, true, {}, QSize(244, 164), 25,
                  {4, 4, 236, 156});
    check(receipts.size() == 2 && receipts[1]["image_cache_key"] == "19" &&
              receipts[1]["mapped"].toBool(),
          "paint evidence distinguishes mapped native skins from unmapped solid fills");
    for (int i = 0; i < 100; ++i)
        trace.observe("pixmap", {220, 0, 180, 160}, {}, false, {}, QSize(244, 164), i + 100);
    check(receipts.size() == 64, "diagnostic receipts are capped per binding revision");
    trace.begin({region}, {}, 2);
    trace.observe("image", {220, 0, 180, 160}, {}, false);
    check(receipts.size() == 65 && receipts.last()["binding_revision"] == 2,
          "a new paint assignment opens a new bounded capture budget");
    Unbound::ClipPaintTrace disabled(false, [&](const auto &value) { receipts.append(value); });
    disabled.begin({region}, {}, 3);
    disabled.observe("image", {220, 0, 180, 160}, {}, false);
    check(receipts.size() == 65 && !disabled.active(),
          "disabled paint evidence emits nothing and retains no capture work");
}
} // namespace OpenGateTest
