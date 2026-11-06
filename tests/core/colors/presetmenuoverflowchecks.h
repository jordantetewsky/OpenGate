// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colorpresetmenu.h"
#include "../workspace-callers/timelineviewportfixtures.h"
#include <QCommonStyle>
#include <QStyleOption>
#include <memory>

namespace OpenGateTest {
// A foreign native style that does not opt into QMenu's scroller itself.
class NonScrollingMenuStyle : public QCommonStyle {
  public:
    mutable int nativeRowsPainted = 0;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *result = nullptr) const override {
        return hint == SH_Menu_Scrollable ? 0
                                         : QCommonStyle::styleHint(hint, option, widget, result);
    }
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override {
        nativeRowsPainted += element == CE_MenuItem;
        QCommonStyle::drawControl(element, option, painter, widget);
    }
};

template <class Check> void presetMenuOverflowChecks(Check check, const QString &directory) {
    Unbound::ColorPalette palette(directory + "/overflow-colors.json");
    auto data = palette.state();
    data.presets.clear();
    for (int i = 0; i < 80; ++i) {
        Unbound::ColorPreset preset;
        preset.id = QString("overflow-%1").arg(i);
        preset.name = QString("Color choice %1").arg(i);
        preset.category = "custom";
        data.presets.append(preset);
    }
    check(palette.replace(data).isEmpty(), "overflow fixture uses the existing committed palette");
    NonScrollingMenuStyle nativeStyle;
    QPointer<QStyle> foreignStyle = &nativeStyle;
    const auto applicationStyle = QApplication::style();
    QMenu menu;
    menu.setAttribute(Qt::WA_DontShowOnScreen);
    menu.setStyle(&nativeStyle);
    menu.setMaximumHeight(220);
    const QString original;
    menu.setStyleSheet(original);
    auto blue = menu.addAction("Blue");
    blue->setCheckable(true);
    blue->setChecked(true);
    auto orange = menu.addAction("Orange");
    const auto nativeRows = menu.actions();
    int nativeCalls = 0;
    QObject::connect(&menu, &QMenu::triggered, &menu, [&](QAction *) { ++nativeCalls; });
    QString invoked;
    int invocations = 0;
    auto controller = std::make_unique<Unbound::ColorPresetMenu>(nullptr);
    const auto prepare = [&] {
        controller->prepare(&menu, palette, "overflow-79", [](const auto &) { return true; },
                            [&](QString id) { invoked = id; ++invocations; });
    };
    check(!menu.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &menu),
          "foreign style starts with native menu scrolling disabled");
    prepare();
    const auto leased = menu.styleSheet();
    prepare();
    check(menu.styleSheet() == leased &&
              menu.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &menu),
          "preset preparation enables Qt's scroller without stacking stylesheet rules");
    QMenu child(&menu);
    child.setStyle(&nativeStyle);
    check(!child.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &child),
          "scroll hint is scoped to the leased menu rather than its native submenus");
    QAction *last = nullptr;
    for (auto action : menu.actions())
        if (action->objectName() == "openGatePresetColor_overflow-79")
            last = action;
    menu.popup(QPoint(300, 300));
    TimelineViewportFixture::settle();
    const bool initiallyClipped = last && !menu.rect().contains(menu.actionGeometry(last));
    for (int i = 0; last && i < 180 && !menu.rect().contains(menu.actionGeometry(last)); ++i) {
        const auto point = menu.rect().center();
        QWheelEvent wheel(point, menu.mapToGlobal(point), {}, {0, -120}, Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&menu, &wheel);
    }
    TimelineViewportFixture::settle();
    check(initiallyClipped && last && menu.rect().contains(menu.actionGeometry(last)),
          "native wheel scrolling makes the final preset row reachable in an overflowing menu");
    menu.grab();
    check(nativeStyle.nativeRowsPainted > 0 && foreignStyle &&
              QApplication::style() == applicationStyle && menu.actions().mid(0, 2) == nativeRows &&
              blue->isChecked() && orange->text() == "Orange" && last && last->isChecked(),
          "scrolling retains foreign native painting, row order and native/preset checkmarks");
    if (last)
        TimelineViewportFixture::click(&menu, menu.actionGeometry(last).center());
    check(invoked == "overflow-79" && invocations == 1 && nativeCalls == 0,
          "reachable lower preset executes once without native menu activation");
    menu.hide();
    const QString external = "\nQMenu { border-width: 3px; }";
    menu.setStyleSheet(menu.styleSheet() + external);
    controller->hide(&menu);
    check(menu.styleSheet() == original + external &&
              !menu.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &menu),
          "hiding preset rows releases only the owned scroll rule and preserves stylesheet edits");
    prepare();
    controller.reset();
    check(menu.styleSheet() == original + external && menu.actions() == nativeRows &&
              foreignStyle && blue->isChecked(),
          "controller destruction restores the native menu and leaves foreign style alive");
    controller = std::make_unique<Unbound::ColorPresetMenu>(nullptr);
    prepare();
    const QString replaced = "QMenu { margin: 5px; }";
    menu.setStyleSheet(replaced);
    check(!menu.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &menu),
          "foreign stylesheet replacement can remove an active lease's scroll rule");
    prepare();
    check(menu.styleSheet().startsWith(replaced) &&
              menu.style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, &menu),
          "repreparing visible presets restores scrolling over the foreign stylesheet replacement");
    const auto renewed = menu.styleSheet();
    prepare();
    check(menu.styleSheet() == renewed,
          "renewed scrolling does not duplicate its stylesheet rule");
    controller.reset();
    check(menu.styleSheet() == replaced,
          "foreign stylesheet replacement survives scroll lease destruction unchanged");
    controller = std::make_unique<Unbound::ColorPresetMenu>(nullptr);
    auto ephemeral = std::make_unique<QMenu>();
    ephemeral->setStyle(&nativeStyle);
    controller->prepare(ephemeral.get(), palette, {}, [](const auto &) { return true; }, {});
    QPointer<QAction> ownedRow;
    for (auto action : ephemeral->actions())
        if (action->objectName().startsWith("openGatePresetColor_"))
            ownedRow = action;
    ephemeral.reset();
    controller.reset();
    check(!ownedRow && foreignStyle,
          "menu destruction releases owned rows and lease before controller teardown");
}
} // namespace OpenGateTest
