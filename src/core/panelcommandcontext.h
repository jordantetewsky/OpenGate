// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QDialog>
#include <QPointer>
#include <QStringList>
#include "textediting.h"
namespace Unbound {
// Read-only panel resolution from existing command IDs and native/group ancestry.
// No panel registry, event filter, visibility writer or remembered hover target.
class PanelCommandContext {
public:
    struct Target {
        QString panel;
        QPointer<QWidget> surface;
        bool operator==(const Target &other) const { return panel==other.panel && surface==other.surface; }
    };
    static Target capture(const QStringList &ids) {
        auto focus=QApplication::focusWidget();
        if (!focus || !focus->isVisible() || !focus->isEnabled()) return {};
        for (auto widget=focus;widget;widget=widget->parentWidget()) {
            const auto grouped=widget->property("groupPanel").toString();
            if (ids.contains(grouped)) return {grouped,widget};
            if (ids.contains(widget->objectName())) return {widget->objectName(),widget};
            if (widget->isWindow()) break;
        }
        return {};
    }
    static QString blocked() {
        const auto active=QApplication::activeWindow();
        if (!active) return "no-active-window";
        if (qobject_cast<QDialog *>(active) || active->property("unboundShortcutEditor").toBool() || QApplication::activeModalWidget()) return "dialog-active";
        if (QApplication::activePopupWidget()) return "menu-active";
        if (textEditor(QApplication::focusWidget())) return "text-editing";
        return {};
    }
    static bool allows(const QString &scope,const Target &target) {
        return scope.isEmpty() || (!target.panel.isEmpty() && target.surface &&
            target.surface->isVisible() && (scope=="*" || scope==target.panel));
    }
    static bool overlaps(const QString &a,const QString &b) {
        return a.isEmpty() || b.isEmpty() || a=="*" || b=="*" || a==b;
    }
};
}
