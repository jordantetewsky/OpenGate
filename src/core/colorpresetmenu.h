// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "isolatednativeaction.h"
#include "menuactioncolorswatch.h"
namespace Unbound {
// Present committed categories/swatches in an existing native menu. This owns
// only its added rows; assignment, commands and native check state live outside.
class ColorPresetMenu : public QObject {
  public:
    explicit ColorPresetMenu(QObject *owner) : QObject(owner) {}
    ~ColorPresetMenu() override {
        for (const auto &entry : menus)
            dispose(entry);
    }
    void prepare(QMenu *menu, const ColorPalette &palette, const QString &selected,
                 std::function<bool(const ColorPreset &)> enabled,
                 std::function<void(QString)> invoke) {
        if (!menus.contains(menu)) {
            menus.insert(menu, {});
            connect(menu, &QObject::destroyed, this, [this, menu] { dispose(menus.take(menu)); });
        }
        auto &entry = menus[menu];
        entry.invoke = std::move(invoke);
        bool intact = !entry.actions.isEmpty();
        for (auto action : entry.actions)
            intact &= action && menu->actions().contains(action);
        if (entry.revision != palette.revision() || !intact) {
            dispose(entry);
            entry.actions.clear();
            entry.colors.clear();
            entry.revision = palette.revision();
            entry.actions.append(menu->addSeparator());
            for (const auto &category : palette.state().categories) {
                bool populated = false;
                for (const auto &preset : palette.state().presets)
                    if (preset.category == category.id) {
                        if (!populated) {
                            entry.actions.append(menu->addSection(category.name));
                            populated = true;
                        }
                        const auto id = preset.id;
                        auto action =
                            IsolatedNativeAction::create(preset.name, this, [this, menu, id] {
                                const auto callback = menus.value(menu).invoke;
                                if (callback)
                                    callback(id);
                            });
                        action->setCheckable(true);
                        action->setObjectName("openGatePresetColor_" + id);
                        action->setProperty("openGateCommandId",
                                            "clip-color.timeline.preset." + id);
                        action->setToolTip(
                            preset.opaque()
                                ? "Development rendering trial; project saving and native Undo are "
                                  "not connected"
                                : "Translucent native clip rendering is not verified yet");
                        menu->addAction(action);
                        new MenuActionColorSwatch(menu, action, preset);
                        entry.actions.append(action);
                        entry.colors.insert(id, action);
                    }
            }
        }
        for (auto action : entry.actions)
            if (action)
                action->setVisible(true);
        for (auto it = entry.colors.cbegin(); it != entry.colors.cend(); ++it)
            if (it.value()) {
                const auto preset = palette.find(it.key());
                it.value()->setVisible(true);
                it.value()->setEnabled(preset && enabled && enabled(*preset));
                it.value()->setChecked(it.key() == selected);
            }
    }
    void hide(QMenu *menu) {
        for (auto action : menus.value(menu).actions)
            if (action)
                action->setVisible(false);
    }

  private:
    struct Entry {
        quint64 revision = ~quint64(0);
        QVector<QPointer<QAction>> actions;
        QHash<QString, QPointer<QAction>> colors;
        std::function<void(QString)> invoke;
    };
    QHash<QMenu *, Entry> menus;
    static void dispose(const Entry &entry) {
        for (auto action : entry.actions)
            if (action)
                delete action;
    }
};
} // namespace Unbound
