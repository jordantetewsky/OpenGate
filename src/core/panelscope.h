// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
#include "nativerootlayout.h"
namespace Unbound {
// A nested native panel may travel inside a leased owner. The caller must pass
// that actual managed surface; sharing a screen/window title is insufficient.
inline bool panelInManagedScope(QWidget *widget,QWidget *page,QWidget *owner=nullptr) {
    if (!widget || !page || widget==page || widget->isWindow()) return false;
    if (page->isAncestorOf(widget) || NativeRootLayout::ownsParkedWidget(widget,page)) return true;
    return owner && owner!=widget && owner->isAncestorOf(widget) &&
        owner->window()->objectName().startsWith("unboundFloating_") &&
        owner->window()->parentWidget()==page->window();
}
}
