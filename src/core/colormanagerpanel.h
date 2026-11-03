// SPDX-License-Identifier: MIT
#pragma once
#include "colormanagerwidget.h"
#include <QPointer>
namespace Unbound {
// Provide one owned/parked content widget to the existing managed panel session.
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
    ColorManagerWidget *ensure(QWidget *page) {
        if (content)
            return content;
        if (!page)
            return nullptr;
        parking = new QWidget(page);
        parking->setObjectName("openGateColorManagerParking");
        parking->hide();
        content =
            new ColorManagerWidget(parking, qEnvironmentVariable("OPENGATE_COLOR_PALETTE_FILE"));
        content->hide();
        return content;
    }

  private:
    QPointer<ColorManagerWidget> content;
    QPointer<QWidget> parking;
};
} // namespace Unbound
