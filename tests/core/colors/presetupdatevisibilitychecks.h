// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colormanagerwidget.h"
#include "presetpickerchecks.h"
#include <QFormLayout>

namespace OpenGateTest {
template <class Check> void presetUpdateVisibilityChecks(Check check, const QString &directory) {
    Unbound::ColorPalette palette(directory + "/visible-update.json");
    Unbound::ColorPaletteDrafts drafts(nullptr, directory + "/visible-update-drafts.json");
    Unbound::ColorManagerWidget manager(palette, drafts);
    manager.setAttribute(Qt::WA_DontShowOnScreen);
    manager.resize(1000, 700);
    manager.show();
    const auto settle = [] {
        for (int i = 0; i < 8; ++i)
            QApplication::processEvents();
    };
    settle();
    auto editor = dynamic_cast<Unbound::ColorPresetEditor *>(
        manager.findChild<QWidget *>("openGateColorPresetEditor"));
    check(editor != nullptr, "manager exposes its actual preset editor");
    if (!editor)
        return;
    auto update = manager.findChild<QPushButton *>("openGateUpdatePreset");
    const auto visible = [&] {
        return update && update->visibleRegion().contains(update->rect().center());
    };
    check(visible(), "Update Preset stays visible without scrolling the long edit form");
    auto field = editor->findChild<QComboBox *>("openGateGradientStop");
    auto form = editor->findChild<QFormLayout *>();
    auto label = form ? form->labelForField(field) : nullptr;
    check(label && !label->isVisible(),
          "solid swatches hide gradient field labels as well as controls");
    auto texture = editor->findChild<QComboBox *>("openGatePresetTexture");
    check(texture != nullptr, "editor exposes its texture control");
    if (!texture)
        return;
    texture->setCurrentIndex(texture->findData("stipple-coarse"));
    check(update->isEnabled() && editor->modified(),
          "pending appearance edits enable the visible Update action");
    auto scroll = editor->findChild<QScrollArea *>("openGatePresetFormScroll");
    check(scroll != nullptr, "only the form owns its scrollbar, leaving update actions outside it");
    if (scroll) {
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
        settle();
        check(visible(), "Update remains visible at the end of the form");
        scroll->verticalScrollBar()->setValue(0);
        settle();
        check(visible(), "Update remains visible at the start of the form");
    }
    update->click();
    check(palette.find("red")->texture == "stipple-coarse" && !editor->modified() &&
              !update->isEnabled(),
          "visible Update commits through the existing palette owner and clears pending state");
    presetPickerChecks(check, manager, *editor, palette);
    manager.resize(360, 1000);
    settle();
    manager.resize(360, 1000);
    settle();
    check(manager.width() <= 360 && visible(), "narrow dock retains a reachable Update action");
}
} // namespace OpenGateTest
