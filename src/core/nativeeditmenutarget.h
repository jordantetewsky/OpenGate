// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QAccessible>
#include <QLabel>
#include <QMenu>
#include <QWidgetAction>
#include "paintedmenutext.h"
namespace Unbound {
inline QString nativeEditLabel(QString value) {
    value=value.section('\t',0,0); value.remove('&'); return value.trimmed();
}
inline bool nativeEditActionLabel(QAction *action,const QString &expected,bool inspectPaint=false) {
    if (nativeEditLabel(action->text())==expected) return true;
    auto custom=qobject_cast<QWidgetAction *>(action);
    auto contents=custom ? custom->defaultWidget() : nullptr;
    if (!contents) return false;
    // Native custom-painted menu rows need not expose a QLabel/QAbstractButton.
    // Use semantic public metadata; never infer identity from a row index.
    if(nativeEditLabel(contents->accessibleName())==expected ||
       nativeEditLabel(contents->property("text").toString())==expected ||
       nativeEditLabel(contents->property("title").toString())==expected)return true;
    if(auto accessible=QAccessible::queryAccessibleInterface(contents))
        if(nativeEditLabel(accessible->text(QAccessible::Name))==expected)return true;
    QList<QWidget *> widgets{contents}; widgets.append(contents->findChildren<QWidget *>());
    for (auto widget:widgets) {
        if (auto text=qobject_cast<QLabel *>(widget)) if (nativeEditLabel(text->text())==expected) return true;
        if (auto button=qobject_cast<QAbstractButton *>(widget)) if (nativeEditLabel(button->text())==expected) return true;
    }
    if(inspectPaint)for(const auto &text:paintedMenuText(contents))
        if(nativeEditLabel(text)==expected)return true;
    return false;
}
inline bool nativeTimelineOptionsMenu(QMenu *menu) {
    if (menu->property("unboundOwnMenu").toBool()) return false;
    // The native popup's owner is proven by the Resolve click trace. Its rows
    // may be QWidgetActions with empty action text and embedded labels.
    if (menu->parentWidget() && menu->parentWidget()->objectName()=="btnEditToolbarPreferences") return true;
    for (auto action:menu->actions()) if (nativeEditActionLabel(action,"Display Stacked Timelines")) return true;
    return false;
}
}
