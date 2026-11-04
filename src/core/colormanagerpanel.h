// SPDX-License-Identifier: MIT
#pragma once
#include "colormanagerwidget.h"
#include <QPointer>
#include <memory>
namespace Unbound {
// Own the feature lifetime and wire its one shared palette to a parked widget.
// No second floating host, docking engine, input filter or workspace writer.
class ColorManagerPanel {
  public:
    ColorManagerPanel() = default;
    ColorManagerPanel(const ColorManagerPanel &) = delete;
    ColorManagerPanel &operator=(const ColorManagerPanel &) = delete;
    ~ColorManagerPanel() {
        delete content;
        delete parking;
    }
    static QString id() {
        return "openGateColorManagerPanel";
    }
    static QString label() {
        return "Clips & Tracks Color Manager";
    }
    ColorPalette &palette() {
        if (!colors)
            colors =
                std::make_unique<ColorPalette>(qEnvironmentVariable("OPENGATE_COLOR_PALETTE_FILE"));
        return *colors;
    }
    ColorManagerWidget *ensure(QWidget *page) {
        if (content)
            return content;
        if (!page)
            return nullptr;
        parking = new QWidget(page);
        parking->setObjectName("openGateColorManagerParking");
        parking->hide();
        if (!drafts)
            drafts =
                std::make_unique<ColorPaletteDrafts>(nullptr, palette().location() + ".drafts");
        content = new ColorManagerWidget(palette(), *drafts, parking);
        content->hide();
        return content;
    }

  private:
    std::unique_ptr<ColorPalette> colors;
    std::unique_ptr<ColorPaletteDrafts> drafts;
    QPointer<ColorManagerWidget> content;
    QPointer<QWidget> parking;
};
} // namespace Unbound
