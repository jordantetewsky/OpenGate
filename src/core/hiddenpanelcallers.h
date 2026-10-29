// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QToolButton>
#include <QTimer>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Fits existing group headers; no extra row or change to split proportions.
class HiddenPanelCallers : public QObject {
public:
    struct Entry { QString id,title; std::function<void()> reveal; };
    explicit HiddenPanelCallers(QObject *owner) : QObject(owner) {}
    QToolButton *add(QWidget *header) {
        auto button = new QToolButton(header); button->setObjectName("unboundHiddenPanelCaller");
        button->setText("+"); button->setAutoRaise(true); button->setFixedSize(24,24);
        button->setPopupMode(QToolButton::InstantPopup);
        auto menu = new QMenu(button); menu->setProperty("unboundOwnMenu",true); button->setMenu(menu);
        connect(menu,&QMenu::aboutToShow,this,[this,menu] {
            menu->clear();
            for (const auto &entry : hidden) {
                auto action = menu->addAction(entry.title); action->setProperty("unboundPanelId",entry.id);
                connect(action,&QAction::triggered,this,[this,entry] {
                    // Close native menus before layout/visibility changes.
                    QTimer::singleShot(0,this,[entry] {
                        logEvent("panel.hidden-caller.reveal",{{"panel",entry.id},{"origin","group-header"}});
                        if (entry.reveal) entry.reveal();
                    });
                });
            }
        });
        buttons.append(button); updateButton(button); return button;
    }
    void populate(QMenu *menu) {
        if (hidden.isEmpty()) return;
        auto group=menu->addMenu("Show Hidden Panels");group->setProperty("unboundOwnMenu",true);
        for (const auto &entry:hidden) { auto action=group->addAction(entry.title);action->setProperty("unboundPanelId",entry.id);
            connect(action,&QAction::triggered,this,[this,entry] { QTimer::singleShot(0,this,[entry] { if (entry.reveal) entry.reveal(); }); }); }
    }
    void setHidden(QList<Entry> entries) {
        hidden = std::move(entries);
        for (int i=buttons.size()-1; i>=0; --i) {
            if (!buttons[i]) buttons.removeAt(i); else updateButton(buttons[i]);
        }
    }
private:
    void updateButton(QToolButton *button) {
        button->setVisible(!hidden.isEmpty());
        button->setToolTip(QStringLiteral("Show hidden panels in this group (%1)").arg(hidden.size()));
        button->setAccessibleName(button->toolTip());
    }
    QList<Entry> hidden; QList<QPointer<QToolButton>> buttons;
};
}
