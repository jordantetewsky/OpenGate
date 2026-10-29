// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <functional>
namespace Unbound {
// Populate native menus before they are mirrored by Cocoa, not just on QWidget Show.
class WorkspaceCallers : public QObject
{
public:
    struct Command { QString id, label; std::function<bool()> visible; std::function<void()> toggle; std::function<bool()> managed = {}; };
    explicit WorkspaceCallers(QObject *owner) : QObject(owner) {}
    void add(Command command) { commands.append(std::move(command)); }
    void wire()
    {
        for (auto widget : QApplication::allWidgets()) if (auto menu = qobject_cast<QMenu *>(widget))
            if (menu->objectName() == QLatin1String("workspaceShowPanel")) refresh(menu);
    }
private:
    void refresh(QMenu *menu)
    {
        for (int i = 0; i < commands.size(); ++i) {
            const auto &command = commands[i]; QAction *entry = nullptr;
            for (auto action : menu->actions()) if (action->objectName() == command.id) entry = action;
            if (!entry) for (auto action : menu->actions()) {
                QString text = action->text(); text.remove('&');
                if (text.trimmed() == command.label) { entry = action; break; }
            }
            if (!entry) {
                entry = menu->addAction(command.label); entry->setObjectName(command.id); entry->setCheckable(true);
            }
            if (!wiredActions.contains(entry)) {
                wiredActions.insert(entry);
                connect(entry, &QObject::destroyed, this, [this, entry] { wiredActions.remove(entry); });
                if (!wiredMenus.contains(menu)) {
                    wiredMenus.insert(menu);
                    connect(menu, &QObject::destroyed, this, [this, menu] { wiredMenus.remove(menu); });
                    connect(menu, &QMenu::aboutToShow, this, [this, menu] { refresh(menu); });
                }
                connect(entry, &QAction::triggered, this, [this, i] {
                    if (!commands[i].managed || commands[i].managed()) commands[i].toggle();
                });
            }
            if (!command.managed || command.managed()) entry->setChecked(command.visible());
        }
    }
    QSet<QAction *> wiredActions;
    QSet<QMenu *> wiredMenus;
    QList<Command> commands;
};
}
