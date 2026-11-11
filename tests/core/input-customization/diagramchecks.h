// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/speededitordiagram.h"
#include <QApplication>

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
    QString lastSelection;
    diagram.selected = [&](QString id) {
        ++selections;
        lastSelection=id;
    };
    diagram.button("cam_7")->click();
    check(selections == 1 && lastSelection=="cam_7" && diagram.selectedKey() == "cam_7" &&
              diagram.button("cam_7")->isChecked() && !diagram.button("roll")->isChecked(),
          "physical controls share one selected key without dispatching hardware commands");
    check(!diagram.setSelectedKey("missing") && diagram.selectedKey() == "cam_7",
          "unknown control cannot replace selected key");
    diagram.button("cam_7")->click();
    check(selections==2 && lastSelection.isEmpty() && diagram.selectedKey().isEmpty() &&
              !diagram.button("cam_7")->isChecked(),"clicking selected key again clears editor selection");
    diagram.button("dial")->click();diagram.button("dial")->click();
    check(diagram.selectedKey().isEmpty() && !diagram.button("dial")->isChecked(),
          "clicking selected dial again clears its highlight");
    diagram.button("jog")->click();
    QMouseEvent background(QEvent::MouseButtonPress,QPointF(2,2),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&diagram,&background);
    check(diagram.selectedKey().isEmpty() && lastSelection.isEmpty() && !diagram.button("jog")->isChecked(),
          "clicking diagram gray background clears selected control");
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
