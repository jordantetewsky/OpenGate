// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QVariant>

namespace Unbound {
// Lease Qt's native menu scroller without owning the menu's foreign base style.
class MenuScrollableStyleLease {
  public:
    explicit MenuScrollableStyleLease(QMenu *menu)
        : menu(menu), property("openGateMenuScrollable_" + QByteArray::number(quintptr(this), 16)),
          rule("\n/* OpenGate menu scrolling " + QString::fromLatin1(property) +
               " */\nQMenu[" + QString::fromLatin1(property) +
               "=\"true\"] { menu-scrollable: 1; }\n") {}
    ~MenuScrollableStyleLease() { release(); }
    void acquire() {
        if (!menu)
            return;
        if (!active) {
            previous = menu->property(property.constData());
            active = true;
        }
        if (menu->property(property.constData()) != QVariant(true))
            menu->setProperty(property.constData(), true);
        if (!menu->styleSheet().contains(rule))
            menu->setStyleSheet(menu->styleSheet() + rule);
    }
    void release() {
        if (!menu || !active)
            return;
        active = false;
        auto current = menu->styleSheet();
        if (current.contains(rule)) {
            current.remove(rule);
            menu->setStyleSheet(current);
        }
        if (menu->property(property.constData()) == QVariant(true))
            menu->setProperty(property.constData(), previous);
    }

  private:
    Q_DISABLE_COPY(MenuScrollableStyleLease)
    QPointer<QMenu> menu;
    QByteArray property;
    QString rule;
    QVariant previous;
    bool active = false;
};
} // namespace Unbound
