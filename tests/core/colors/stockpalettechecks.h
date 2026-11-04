// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colormanagerwidget.h"
namespace OpenGateTest {
template <class Check> void stockPaletteChecks(Check check, const QString &directory) {
    Unbound::ColorPalette palette(directory + "/protected-colors.json");
    Unbound::ColorPaletteDrafts drafts(nullptr, directory + "/protected-colors.drafts");
    auto custom = palette.state().presets.front();
    custom.id = "custom-comparison";
    custom.name = "My blue";
    custom.color = QColor("#2057b4");
    check(palette.update(custom).isEmpty(),
          "custom swatches remain editable alongside protected native colors");
    const auto original = Unbound::ColorPalette::encode(palette.state());
    const auto revision = palette.revision();
    QObject observer;
    int changes = 0;
    palette.subscribe(&observer, [&] { ++changes; });
    auto reserved = custom;
    reserved.id = "native-blue";
    check(!palette.update(reserved).isEmpty() &&
              Unbound::ColorPalette::encode(palette.state()) == original &&
              palette.revision() == revision && changes == 0,
          "reserved native swatch IDs cannot be inserted or overridden by custom updates");
    auto injected = palette.state();
    injected.categories.append(Unbound::ColorPalette::Category{"stock", "Renamed Resolve colors"});
    check(!palette.import(Unbound::ColorPalette::encode(injected)).isEmpty() &&
              Unbound::ColorPalette::encode(palette.state()) == original,
          "custom import cannot replace the protected native category");
    Unbound::ColorManagerWidget manager(palette, drafts);
    auto stock = manager.findChild<QComboBox *>("openGateStockColorComparison");
    auto editor = dynamic_cast<Unbound::ColorPresetEditor *>(
        manager.findChild<QWidget *>("openGateColorPresetEditor"));
    if (!editor)
        for (auto candidate : manager.findChildren<QWidget *>())
            if ((editor = dynamic_cast<Unbound::ColorPresetEditor *>(candidate)))
                break;
    check(stock && stock->count() == 16 && !stock->isEditable(),
          "manager exposes the complete native comparison palette without editable rows");
    QVector<QPair<QString, QColor>> baseline;
    if (stock)
        for (int i = 0; i < stock->count(); ++i)
            baseline.append({stock->itemText(i), stock->itemData(i).value<QColor>()});
    const auto draft = editor ? editor->value() : Unbound::ColorPreset();
    if (stock)
        stock->setCurrentIndex(0);
    check(editor && editor->value() == draft &&
              Unbound::ColorPalette::encode(palette.state()) == original,
          "selecting a native comparison never edits or commits a custom preset");
    QPushButton *remove = nullptr;
    for (auto button : manager.findChildren<QPushButton *>())
        if (button->text() == "Remove Color")
            remove = button;
    if (remove)
        remove->click();
    bool unchanged = stock && stock->count() == baseline.size();
    if (stock)
        for (int i = 0; i < baseline.size(); ++i)
            unchanged &= stock->itemText(i) == baseline[i].first &&
                         stock->itemData(i).value<QColor>() == baseline[i].second;
    check(remove && palette.state().presets.size() == 1 && unchanged,
          "Remove Color deletes only the selected custom preset and preserves every native swatch");
}
} // namespace OpenGateTest
