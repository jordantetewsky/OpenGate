// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include "nativevieweractions.h"

namespace Unbound {
// Command discovery follows menu lifetime, rather than scanning every widget
// each time a panel opens or closes. State/shortcut values remain on QAction.
class ViewerActionRegistry : public QObject
{
public:
    explicit ViewerActionRegistry(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~ViewerActionRegistry() override { qApp->removeEventFilter(this); }
    QList<QAction *> actions(QWidget *window)
    {
        if (main != window) { main = window; dirty = true; }
        if (dirty) {
            dirty = false;
            cached.clear();
            for (auto action : nativeViewerModeActions(window)) cached.append(action);
        }
        QList<QAction *> result;
        for (const auto &action : cached) if (action && viewerCommandLabel(action) == QLatin1String("Single Viewer Mode")) result.append(action);
        return result;
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved || event->type() == QEvent::LanguageChange)
            dirty = true;
        return false;
    }
private:
    QPointer<QWidget> main;
    QList<QPointer<QAction>> cached;
    bool dirty = true;
};
}
