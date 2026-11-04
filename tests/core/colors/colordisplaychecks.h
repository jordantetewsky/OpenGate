// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colormanagerwidget.h"
#include "../../../src/core/colortexturetoolbar.h"
namespace OpenGateTest {
template <class Check> void colorDisplayChecks(Check check, const QString &directory) {
    const auto path = directory + "/local-display.json";
    Unbound::ColorDisplaySettings settings(nullptr, path);
    QObject observer;
    int changes = 0;
    settings.subscribe(&observer, [&](bool) { ++changes; });
    check(settings.texturesEnabled() && settings.setTexturesEnabled(false).isEmpty() &&
              !settings.texturesEnabled() && changes == 1,
          "texture display changes once after atomic local preference save");
    settings.setTexturesEnabled(false);
    check(changes == 1, "repeated texture setting is a no-op");
    Unbound::ColorDisplaySettings reopened(nullptr, path);
    check(!reopened.texturesEnabled(), "texture display preference survives local reopen");
    const auto bad = directory + "/invalid-display.json";
    QFile broken(bad);
    broken.open(QIODevice::WriteOnly);
    broken.write("{invalid");
    broken.close();
    Unbound::ColorDisplaySettings corrupt(nullptr, bad);
    check(!corrupt.setTexturesEnabled(false).isEmpty() && corrupt.texturesEnabled(),
          "invalid display preference preserves file and active state");
    broken.open(QIODevice::ReadOnly);
    check(broken.readAll() == "{invalid", "invalid color display file is never overwritten");
    broken.close();
    auto display = Unbound::ColorDisplaySettings::shared();
    display->setTexturesEnabled(true);
    Unbound::ColorPalette palette(directory + "/display-palette.json");
    Unbound::ColorPaletteDrafts drafts(nullptr, directory + "/display-drafts.json");
    const auto saved = Unbound::ColorPalette::encode(palette.state());
    Unbound::ColorManagerWidget manager(palette, drafts);
    auto checkbox = manager.findChild<QCheckBox *>("openGateColorTexturesEnabled");
    QWidget toolbar;
    auto layout = new QHBoxLayout(&toolbar);
    auto native = new QPushButton("Native command", &toolbar);
    layout->addWidget(native);
    int nativeCalls = 0;
    QObject::connect(native, &QPushButton::clicked, [&] { ++nativeCalls; });
    Unbound::ColorTextureToolbar controls(&toolbar, *display, [display] {
        display->setTexturesEnabled(!display->texturesEnabled());
    });
    controls.wire(&toolbar);
    controls.wire(&toolbar);
    auto button = toolbar.findChild<QToolButton *>("openGateTimelineTextures");
    check(button && toolbar.findChildren<QToolButton *>("openGateTimelineTextures").size() == 1 &&
              button->focusPolicy() == Qt::NoFocus,
          "texture toolbar attaches one focus-preserving control");
    if (checkbox)
        checkbox->click();
    check(checkbox && button && !display->texturesEnabled() && !checkbox->isChecked() &&
              !button->isChecked(),
          "manager checkbox and toolbar reflect one shared texture state");
    if (button)
        button->click();
    check(display->texturesEnabled() && checkbox && checkbox->isChecked() && nativeCalls == 0 &&
              native->parentWidget() == &toolbar,
          "toolbar restores textures and synchronizes manager without native command side effects");
    check(Unbound::ColorPalette::encode(palette.state()) == saved,
          "texture display changes preserve preset definitions and category ordering");
}
} // namespace OpenGateTest
