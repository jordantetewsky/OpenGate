// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colormanagerwidget.h"

namespace OpenGateTest {
template <class Check>
void presetPickerChecks(Check check, Unbound::ColorManagerWidget &manager,
                        Unbound::ColorPresetEditor &editor, Unbound::ColorPalette &palette) {
    const auto settle = [] {
        for (int i = 0; i < 8; ++i)
            QApplication::processEvents();
    };
    auto pick = dynamic_cast<Unbound::ColorPickerButton *>(
        editor.findChild<QWidget *>("openGateColorPickerButton"));
    auto sample = manager.findChild<QWidget *>("openGateColorPresetPreview");
    check(pick && sample, "assembled editor exposes a clickable color pill and its sample preview");
    if (!pick || !sample)
        return;
    const auto original = editor.value();
    const auto before = sample->grab().toImage();
    pick->click();
    settle();
    QPointer<QWidget> popup = pick->findChild<QWidget *>("openGateColorPickerPopup");
    auto plane = popup ? popup->findChild<QWidget *>("openGateColorPickerPlane") : nullptr;
    check(popup && popup->isVisible() && plane,
          "clicking the color pill opens its compact picker beside the editor");
    if (!popup || !plane)
        return;
    const QPointF at(plane->width() * .25, plane->height() * .35);
    QMouseEvent drag(QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(plane, &drag);
    settle();
    check(editor.modified() && editor.value().color != original.color &&
              *palette.find(original.id) == original && sample->grab().toImage() != before,
          "picker drag repaints the real sample immediately without committing the palette");
    auto cancel = popup->findChild<QPushButton *>("openGatePickerCancel");
    check(cancel != nullptr, "live picker offers Cancel");
    if (cancel)
        cancel->click();
    settle();
    check(editor.value() == original && !editor.modified() && sample->grab().toImage() == before,
          "Cancel restores the exact draft color and sample without changing saved paint");
    auto gradient = original;
    gradient.stops = {{"left", Qt::red, 0, 1}, {"right", Qt::blue, 1, 1}};
    gradient.color = Qt::red;
    check(palette.update(gradient).isEmpty(), "gradient fixture uses the existing palette owner");
    editor.setPreset(gradient);
    auto stop = editor.findChild<QComboBox *>("openGateGradientStop");
    stop->setCurrentIndex(1);
    pick->click();
    settle();
    popup = pick->findChild<QWidget *>("openGateColorPickerPopup");
    // Retired popups are deleted at the normal deferred-delete event boundary.
    for (auto candidate : pick->findChildren<QWidget *>("openGateColorPickerPopup"))
        if (candidate->isVisible())
            popup = candidate;
    auto hex = popup ? popup->findChild<QLineEdit *>("openGatePickerHex") : nullptr;
    check(hex && pick->accessibleName().startsWith("Gradient stop 2 of 2"),
          "gradient picker identifies the selected stop rather than editing the whole gradient");
    if (!hex)
        return;
    hex->selectAll();
    QKeyEvent typed(QEvent::KeyPress, 0, Qt::NoModifier, "#7138AF");
    QApplication::sendEvent(hex, &typed);
    settle();
    check(editor.value().stops[0].color == gradient.stops[0].color &&
              editor.value().stops[1].color == QColor("#7138af") &&
              *palette.find(gradient.id) == gradient,
          "live picker edits only the selected gradient stop and leaves saved settings intact");
    if (const auto snapshot = qEnvironmentVariable("OPENGATE_COLOR_PICKER_SNAPSHOT");
        !snapshot.isEmpty())
        check(popup->grab().save(snapshot), "compact picker screenshot writes successfully");
    auto done = popup->findChild<QPushButton *>("openGatePickerDone");
    check(done != nullptr, "live picker offers Done");
    if (done)
        done->click();
    settle();
    check(editor.modified() && *palette.find(gradient.id) == gradient,
          "Done retains the preview draft and does not silently save the preset");
    editor.findChild<QPushButton *>("openGateUpdatePreset")->click();
    check(!editor.modified() && palette.find(gradient.id)->stops[1].color == QColor("#7138af"),
          "Update Preset commits the selected-stop edit through the existing palette owner");
    const auto saved = editor.value();
    pick->click();
    settle();
    for (auto candidate : pick->findChildren<QWidget *>("openGateColorPickerPopup"))
        if (candidate->isVisible())
            popup = candidate;
    hex = popup->findChild<QLineEdit *>("openGatePickerHex");
    hex->selectAll();
    QKeyEvent nextColor(QEvent::KeyPress, 0, Qt::NoModifier, "#22AA44");
    QApplication::sendEvent(hex, &nextColor);
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(hex, &escape);
    settle();
    check(editor.value() == saved && (!popup || !popup->isVisible()),
          "Escape from the picker text field cancels that pick without reverting other edits");
}
} // namespace OpenGateTest
