// SPDX-License-Identifier: MIT
#pragma once
#include <QBoxLayout>
#include <QPointer>
#include <QSet>
#include <QStackedWidget>
#include <QToolButton>
#include <QTimer>
#include "diagnostics.h"
#include "panelplacementcontrol.h"
#include "../platform/workspaceuiconventions.h"
namespace Unbound {
inline QWidget *panelSurface(QWidget *page, const QString &name)
{
    if (!page) return nullptr;
    if (name != QLatin1String("unboundWorkspaceToolbar")) {
        auto result = page->findChild<QWidget *>(name);
        if (!result) result = page->window()->findChild<QWidget *>(name);
        // The offline source stack can exist before Resolve constructs a loaded
        // source player. It is itself the stable surface we detach, not a clip.
        if (!result && name == QLatin1String("widgetEditSourcePlayer")) {
            result = page->findChild<QStackedWidget *>("stackedEditSourceOfflinePlayers");
            if (!result) result = page->window()->findChild<QStackedWidget *>("stackedEditSourceOfflinePlayers");
        }
        if(!result) {
            const auto alias=nativeWorkspacePanelAlias(name);
            if(!alias.isEmpty()) {
                result=page->findChild<QWidget *>(alias);
                if(!result)result=page->window()->findChild<QWidget *>(alias);
            }
        }
        return result;
    }
    // Keep both native single/dual-screen toolbar modes together.
    auto mode = page->findChild<QWidget *>("pageSingleScreenModeTools");
    if(!mode)for(auto stash:page->window()->findChildren<QWidget *>("unboundNativeLayoutStash"))
        if(stash->property("unboundParkedNativePage").value<QObject *>()==page){mode=stash->findChild<QWidget *>("pageSingleScreenModeTools");if(mode)break;}
    if (!mode) mode = page->window()->findChild<QWidget *>("pageSingleScreenModeTools");
    return mode ? qobject_cast<QStackedWidget *>(mode->parentWidget()) : nullptr;
}
class ToolbarDecoration : public QObject
{
public:
    explicit ToolbarDecoration(QObject *owner) : QObject(owner) {}
    void wire(QWidget *page, const QString &panel, std::function<void()> toggle)
    {
        auto surface = panelSurface(page, panel); if (!surface) return;
        QList<QWidget *> rows;
        if (panel == QLatin1String("frameEditToolbarTop")) rows.append(surface);
        else for (auto widget : surface->findChildren<QWidget *>())
            if (widget->objectName().startsWith("frameEditTopBarLeft")) rows.append(widget);
        for (auto row : rows) {
            const auto id = QStringLiteral("unboundUndockToolbar_") + panel;
            auto button = row->findChild<QToolButton *>(id);
            if (!button) {
                button = new QToolButton(row); button->setObjectName(id);
                button->setAutoRaise(true); button->setFocusPolicy(Qt::NoFocus);
            }
            if (row->layout()) row->layout()->removeWidget(button);
            if (!row->property("unboundToolbarActionSpace").toBool()) {
                auto margins = row->contentsMargins(); margins.setRight(margins.right() + 24);
                row->setContentsMargins(margins); row->setProperty("unboundToolbarActionSpace", true);
            }
            row->installEventFilter(this); position(row, button); button->show(); button->raise();
            const bool floating = surface->window()->objectName().startsWith("unboundFloating_");
            button->setText(floating ? QStringLiteral("↙") : QStringLiteral("↗"));
            button->setToolTip(floating ? (panel == "frameEditToolbarTop" ? "Attach tools to Timeline" : "Attach toolbar above Timeline; drag to place elsewhere") : "Float this toolbar");
            if (panel=="unboundWorkspaceToolbar") {
                const bool standalone=surface->window()->property("unboundToolbarStrip").toBool();
                button->setText(standalone ? QStringLiteral("↙") : QStringLiteral("↗"));
                button->setToolTip(standalone ? "Attach toolbar above Timeline; drag to place elsewhere" : "Float toolbar separately");
            }
            setPlacementControl(button,panel=="unboundWorkspaceToolbar" ? surface->window()->property("unboundToolbarStrip").toBool() : floating,button->toolTip());
            if (wired.contains(button)) continue;
            wired.insert(button);
            connect(button, &QObject::destroyed, this, [this, button] { wired.remove(button); });
            connect(button, &QToolButton::clicked, this, [this, panel, toggle] {
                logEvent(QStringLiteral("toolbar.undock.clicked"), {{"panel", panel}});
                QTimer::singleShot(0, this, toggle);
            });
        }
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        auto row = qobject_cast<QWidget *>(object);
        if (row && event->type() == QEvent::Resize)
            for (auto button : row->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly))
                if (button->objectName().startsWith("unboundUndockToolbar_")) position(row, button);
        return false;
    }
private:
    static void position(QWidget *row, QToolButton *button)
    { button->setGeometry(row->width() - 24, qMax(0, (row->height() - 24) / 2), 24, 24); }
    QSet<QToolButton *> wired;
};
}
