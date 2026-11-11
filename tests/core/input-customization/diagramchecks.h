// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/speededitordiagram.h"

namespace OpenGateTest {
template <typename Check> void speedEditorDiagramChecks(Check check) {
    Unbound::SpeedEditorDiagram diagram;
    using Tone = Unbound::SpeedEditorKey::Tone;
    check(diagram.keyIds().size() == 44, "diagram exposes 43 physical keys and one dial");
    check(diagram.button("in")->keyTone() == Tone::White &&
              diagram.button("out")->keyTone() == Tone::White &&
              diagram.button("shtl")->keyTone() == Tone::White &&
              diagram.button("jog")->keyTone() == Tone::White &&
              diagram.button("scrl")->keyTone() == Tone::White &&
              diagram.button("full_view")->keyTone() == Tone::Red &&
              diagram.button("dial")->keyTone() == Tone::Dial,
          "distinct hardware control colors are preserved");
    int selections = 0;
    diagram.selected = [&](QString id) {
        ++selections;
        check(id == "cam_7", "camera selection publishes its stable owned ID");
    };
    diagram.button("cam_7")->click();
    check(selections == 1 && diagram.selectedKey() == "cam_7" &&
              diagram.button("cam_7")->isChecked() && !diagram.button("roll")->isChecked(),
          "physical controls share one selected key without dispatching hardware commands");
    check(!diagram.setSelectedKey("missing") && diagram.selectedKey() == "cam_7",
          "unknown control cannot replace selected key");
    bool accessible = true;
    for (const auto &id : diagram.keyIds()) {
        const auto key = diagram.button(id);
        accessible &= !key->accessibleName().isEmpty() && key->focusPolicy() == Qt::StrongFocus;
    }
    check(accessible, "every physical control exposes keyboard focus and an accessible name");
    const auto before = diagram.minimumSizeHint();
    auto font = diagram.font();
    if (font.pixelSize() > 0) font.setPixelSize(font.pixelSize() * 2);
    else font.setPointSizeF(font.pointSizeF() * 2);
    diagram.setFont(font);
    check(diagram.minimumSizeHint().width() > before.width() * 1.8,
          "minimum device geometry follows native font scaling");
    diagram.resize(diagram.minimumSizeHint());
    // QWidget geometry is laid out on first show, just as in the actual tab.
    diagram.setAttribute(Qt::WA_DontShowOnScreen);
    diagram.show();
    check(diagram.button("cam_7")->y() < diagram.button("cam_1")->y() &&
              diagram.button("source")->x() > diagram.button("full_view")->x() &&
              diagram.button("dial")->y() > diagram.button("jog")->y(),
          "camera grid and dial retain physical device placement");
}
} // namespace OpenGateTest
