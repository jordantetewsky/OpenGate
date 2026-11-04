// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QJsonArray>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QSet>
#include <QScopedValueRollback>
#include "diagnostics.h"
#include "workflowcommandidentity.h"
namespace Unbound {
// Observe Resolve-owned commands without changing menus, ownership, shortcuts or
// associated widgets. In particular, never mutate menus during native tracking.
class WorkflowCommandMenu : public QObject {
public:
    explicit WorkflowCommandMenu(QObject *owner):QObject(owner) { qApp->installEventFilter(this); }
    ~WorkflowCommandMenu() override { if (qApp) qApp->removeEventFilter(this); }
    static void prepareKeyboardEditor() {} // Native commands already remain in place.
    void restoreForEditor() {} // Compatibility with existing callers; no restoration needed.
    void wire(QMenu *root) {
        Q_UNUSED(root);
        if (wiring || !dirty) return;
        QScopedValueRollback<bool> guard(wiring,true);
        dirty=false;
        QList<QPointer<QWidget>> widgets;
        for (auto widget:QApplication::allWidgets()) widgets.append(widget);
        logEvent("command.menu.discovery",{{"widget_count",widgets.size()}});
        for (const auto &widget:widgets) if (auto menu=qobject_cast<QMenu *>(widget.data())) {
            QList<QPointer<QAction>> actions;
            for (auto action:menu->actions()) actions.append(action);
            for (const auto &action:actions) {
                if (!action || !nativeOpenGateWorkflowAction(action)) continue;
                if (!observed.contains(action)) {
                    observed.insert(action);
                    connect(action,&QAction::changed,this,[this,action] { if (action) recordBinding(action); });
                    auto identity=action.data();
                    connect(action,&QObject::destroyed,this,[this,identity] {
                        observed.remove(identity); bindingStates.remove(identity);
                    });
                    connect(action,&QAction::triggered,this,[action] {
                        if (action) logEvent("command.native.action",{{"action",action->objectName()},
                            {"label",action->text()},{"route","resolve-workflow-handler"}});
                    });
                    logEvent("command.menu.observed",{{"action",action->objectName()},
                        {"label",action->text()},{"menu",menu->objectName()},
                        {"mode","native-unmodified"}});
                }
                recordBinding(action);
            }
        }
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::ActionAdded || event->type()==QEvent::ActionRemoved)
            if(auto menu=qobject_cast<QMenu *>(object)) {
                // OpenGate rebuilds its presentation menus during state sync.
                // Those changes cannot add Resolve-owned workflow commands.
                bool owned=false;
                for(auto parent=menu;parent;parent=qobject_cast<QMenu *>(parent->parent()))
                    owned |= parent->property("unboundOwnMenu").toBool();
                if(!owned)dirty=true;
            }
        if (event->type()==QEvent::Shortcut)
            if (auto action=qobject_cast<QAction *>(object)) if (nativeOpenGateWorkflowAction(action))
                logEvent("command.native.shortcut",{{"action",action->objectName()},{"label",action->text()}});
        return false;
    }
private:
    void recordBinding(QAction *action) {
        if (!action) return;
        QStringList keys,hosts;
        for (auto key:action->shortcuts()) keys.append(key.toString(QKeySequence::PortableText));
        for (auto host:action->associatedWidgets()) if (host->isWindow() && !qobject_cast<QMenu *>(host)) hosts.append(host->objectName());
        hosts.sort();
        const auto signature=keys.join('|')+":"+hosts.join('|')+":"+QString::number(action->shortcutContext());
        if (bindingStates.value(action)==signature) return;
        bindingStates[action]=signature;
        logEvent("command.native.binding",{{"action",action->objectName()},{"label",action->text()},
            {"keys",QJsonArray::fromStringList(keys)},{"hosts",QJsonArray::fromStringList(hosts)},
            {"context",int(action->shortcutContext())},{"binding_store","resolve-native"}});
    }
    QSet<QAction *> observed;
    QHash<QAction *,QString> bindingStates;
    bool wiring=false,dirty=true;
};
}
