// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>
#include <QApplication>
#include <QJsonObject>
#include <QKeySequence>
#include <QSet>
#include <QShortcut>
#include "panelcommandcontext.h"
#include "nativeshortcutcontext.h"
namespace Unbound {
// Binding validation and native conflict inventory, outside the input hot path.
class CommandBindingConflicts {
public:
    struct Binding { QString id,scope; };
    static QHash<QString,QString> evaluate(const QJsonObject &proposed,const QList<Binding> &commands) {
        QHash<QString,QString> errors; QHash<QString,QStringList> seen;
        bool assigned=false;
        for (const auto &command : commands) assigned |= !proposed.value(command.id).toString().isEmpty();
        if (!assigned) return errors;
        // One native inventory per refresh/save, never one scan per command/key.
        struct Native { QKeySequence key; QString label; };
        QList<Native> nativeKeys; QSet<QAction *> visited;
        for (auto widget : QApplication::allWidgets()) {
            for (auto action : widget->actions()) if (!visited.contains(action)) {
                visited.insert(action);
                if (action->isEnabled() && action->isVisible())
                    for (const auto &key:action->shortcuts()) nativeKeys.append({key,action->objectName()});
            }
            for (auto shortcut : widget->findChildren<QShortcut *>(QString(),Qt::FindDirectChildrenOnly))
                if (shortcut->isEnabled()) nativeKeys.append({shortcut->key(),shortcut->objectName()});
        }
        for (const auto &command : commands) {
            const auto text=proposed.value(command.id).toString(); if (text.isEmpty()) continue;
            const auto key=QKeySequence::fromString(text,QKeySequence::PortableText);
            if (key.isEmpty() || key.count()!=1) { errors[command.id]="Use one key combination per command."; continue; }
            const auto normalized=key.toString(QKeySequence::PortableText);
            for (const auto &other : seen.value(normalized)) if (PanelCommandContext::overlaps(command.scope,scopeOf(commands,other))) {
                const auto message="Two OpenGate commands use "+normalized+" in overlapping contexts.";
                errors[command.id]=message; errors[other]=message;
            }
            seen[normalized].append(command.id);
            for (const auto &native : nativeKeys)
                if (NativeShortcutContext::overlaps(native.label,command.scope) && !native.key.isEmpty() &&
                    (key.matches(native.key)!=QKeySequence::NoMatch || native.key.matches(key)!=QKeySequence::NoMatch)) {
                    errors[command.id]=normalized+" is already assigned to a native action or shortcut."; break;
                }
        }
        return errors;
    }
private:
    static QString scopeOf(const QList<Binding> &commands,const QString &id) {
        for (const auto &command:commands) if (command.id==id) return command.scope;
        return {};
    }
};
}
