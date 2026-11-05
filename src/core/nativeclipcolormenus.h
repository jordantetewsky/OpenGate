// SPDX-License-Identifier: MIT
#pragma once
#include "menuoverflowscroll.h"
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <functional>
namespace Unbound {
// Integrate native clip-color menus, retaining the originating widget across
// nested popup focus changes. No clip lookup, painting or command execution.
class NativeClipColorMenus : public QObject {
  public:
    using Prepare = std::function<void(QMenu *, QWidget *)>;
    NativeClipColorMenus(QObject *owner, Prepare prepare)
        : QObject(owner), prepare(std::move(prepare)) {
        origin = QApplication::focusWidget();
        qApp->installEventFilter(this);
        connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *next) {
            if (next && !qobject_cast<QMenu *>(next) && !QApplication::activePopupWidget())
                origin = next;
        });
    }
    ~NativeClipColorMenus() override {
        if (qApp)
            qApp->removeEventFilter(this);
    }
    static bool matches(QMenu *menu) {
        if (!menu)
            return false;
        auto purpose = label(menu->menuAction()->text());
        // Some native builders attach a separate QAction to an unnamed menu.
        // The parent row describes its scope more accurately than its swatches.
        if (auto parent = qobject_cast<QMenu *>(menu->parentWidget()))
            for (auto action : parent->actions())
                if (action->menu() == menu && !label(action->text()).isEmpty()) {
                    purpose = label(action->text());
                    break;
                }
        if (menu->objectName() == "changeTrackColor" || purpose == "track color" ||
            purpose == "change track color")
            return false;
        if (menu->objectName() == "markSetClipColor")
            return true;
        if (!purpose.isEmpty())
            return purpose == "clip color";
        // Retain late-populated unnamed clip palettes, but never use this
        // swatch-only fallback against a menu with a declared different scope.
        QSet<QString> names;
        for (auto action : menu->actions())
            names.insert(label(action->text()));
        return names.contains("orange") && names.contains("apricot") && names.contains("chocolate");
    }

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::ContextMenu)
            if (auto widget = qobject_cast<QWidget *>(object))
                if (!qobject_cast<QMenu *>(widget) && contextEvent != event) {
                    // One context event may bubble through the native view's
                    // ancestors. Preserve its first receiver, not the last parent.
                    contextEvent = event;
                    origin = widget;
                    QTimer::singleShot(0, this, [this] { contextEvent = nullptr; });
                }
        if (event->type() != QEvent::Show)
            return false;
        auto menu = qobject_cast<QMenu *>(object);
        if (!menu)
            return false;
        if (!watched.contains(menu)) {
            watched.insert(menu);
            connect(menu, &QObject::destroyed, this, [this, menu] { watched.remove(menu); });
            connect(menu, &QMenu::aboutToShow, this, [this, menu] { refresh(menu); });
        }
        refresh(menu);
        // Native context menus can populate their actions in their Show handler.
        QPointer<QMenu> alive = menu;
        QTimer::singleShot(0, this, [this, alive] {
            if (alive && alive->isVisible())
                refresh(alive);
        });
        return false;
    }

  private:
    QPointer<QWidget> origin;
    QSet<QMenu *> watched;
    Prepare prepare;
    const QEvent *contextEvent = nullptr;
    static QString label(QString text) {
        text.remove('&');
        return text.trimmed().toLower();
    }
    void refresh(QMenu *menu) {
        if (matches(menu) && prepare) {
            prepare(menu, origin);
            MenuOverflowScroll::refresh(menu);
        }
    }
};
} // namespace Unbound
