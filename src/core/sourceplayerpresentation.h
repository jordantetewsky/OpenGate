// SPDX-License-Identifier: MIT
#pragma once
#include <QStackedWidget>
#include <QJsonArray>
#include "diagnostics.h"
#include "cinemaviewer.h"
namespace Unbound {
// QStackedWidget::show() does not unhide a current page that native single-view
// preparation explicitly hid. Reconcile only the existing selected page after
// ownership transfer; never select footage, switch pages, or recreate a viewer.
inline void reconcileSourcePlayerPresentation(QWidget *content,const QString &boundary) {
    auto stack=qobject_cast<QStackedWidget *>(content);
    if (!stack || stack->objectName()!="stackedEditSourceOfflinePlayers") return;
    QJsonArray pages;
    for (int i=0;i<stack->count();++i) {
        auto page=stack->widget(i);
        pages.append(QJsonObject{{"index",i},{"widget",widgetState(page)},{"selected",page==stack->currentWidget()}});
    }
    auto selected=stack->currentWidget();
    const bool eligible=stack->isVisible() && selected && selected->parentWidget()==stack && !visibleCinemaViewer();
    const bool hidden=selected && selected->isHidden();
    if (eligible && hidden) {
        selected->show();
        if (selected->layout()) { selected->layout()->invalidate(); selected->layout()->activate(); }
        selected->update();
    }
    logEvent("source.presentation.boundary",{{"boundary",boundary},{"stack",widgetState(stack)},
        {"current_index",stack->currentIndex()},{"pages_before",pages},{"selected_page",widgetState(selected)},
        {"selected_page_revealed",eligible && hidden},{"cinema_viewer",bool(visibleCinemaViewer())}});
}
}
