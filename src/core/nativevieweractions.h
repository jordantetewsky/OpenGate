// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QSet>

namespace Unbound {
inline QString viewerCommandLabel(const QAction *action)
{
    QString label = action->text().section('\t', 0, 0); label.remove('&');
    return label.trimmed();
}
inline QList<QAction *> nativeViewerModeActions(QWidget *main)
{
    if (!main) return {};
    auto candidates = main->findChildren<QAction *>();
    // Resolve's native menu actions need not be QObject children of UiMainWindow,
    // and Cocoa menu object names do not consistently use a lowercase prefix.
    for (auto widget : QApplication::allWidgets())
        if (auto menu = qobject_cast<QMenu *>(widget)) candidates.append(menu->actions());
    QSet<QAction *> seen; QList<QAction *> result;
    for (auto action : candidates)
        if (!seen.contains(action) && !action->menu() && !action->objectName().startsWith("unbound") &&
            viewerCommandLabel(action) == QLatin1String("Single Viewer Mode")) {
            seen.insert(action); result.append(action);
        }
    return result;
}
}
