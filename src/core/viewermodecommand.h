// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QHash>
#include <functional>
#include <QTimer>
#include "diagnostics.h"
#include "nativevieweractions.h"
#include "vieweractionregistry.h"
namespace Unbound {
// Follow the native command identity and retain its user-assigned shortcut.
class ViewerModeCommand : public QObject
{
public:
    ViewerModeCommand(QObject *owner, std::function<bool()> managed, std::function<void(bool)> apply)
        : QObject(owner), managed(std::move(managed)), apply(std::move(apply)), registry(this) {}
    ~ViewerModeCommand() override
    {
        for (auto action : priorEnabled.keys()) action->setEnabled(priorEnabled.value(action));
    }
    void refresh(QWidget *main)
    {
        if (!main) return;
        for (auto action : registry.actions(main)) {
            if (!discovered.contains(action)) {
                discovered.insert(action);
                connect(action, &QObject::destroyed, this, [this, action] { discovered.remove(action); });
                logEvent(QStringLiteral("viewer.mode.discovered"), {{"name", action->objectName()},
                    {"label", action->text()}, {"checkable", action->isCheckable()}, {"checked", action->isChecked()},
                    {"enabled", action->isEnabled()}, {"shortcut", action->shortcut().toString()}, {"managed", managed()}});
            }
            if (!action->isCheckable()) continue;
            if (!observed.contains(action)) {
                observed.insert(action);
                connect(action, &QObject::destroyed, this, [this, action] { observed.remove(action); priorEnabled.remove(action); });
                connect(action, &QAction::triggered, this, [this, action] {
                    if (!managed()) return;
                    const bool single = action->isChecked();
                    QTimer::singleShot(0, this, [this, single] {
                        if (!managed()) return;
                        apply(single);
                        logEvent(QStringLiteral("viewer.mode.command"), {{"single_viewer", single}});
                    });
                });
            }
            if (managed() && !QApplication::activeModalWidget()) {
                if (!priorEnabled.contains(action)) priorEnabled[action] = action->isEnabled();
                action->setEnabled(true);
            } else if (priorEnabled.contains(action)) action->setEnabled(priorEnabled.take(action));
        }
    }
private:
    std::function<bool()> managed;
    std::function<void(bool)> apply;
    ViewerActionRegistry registry;
    QSet<QAction *> observed;
    QSet<QAction *> discovered;
    QHash<QAction *, bool> priorEnabled;
};
}
