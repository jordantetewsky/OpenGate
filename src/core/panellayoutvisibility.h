// SPDX-License-Identifier: MIT
#pragma once
#include <functional>
#include "groupvisibility.h"
namespace Unbound {
// Collapse empty split/tab branches, retaining the structure for later reveal.
// Logical visibility is distinct from being the currently selected tab page.
inline bool applyPanelLayoutVisibility(QWidget *node,GroupVisibility &state,const std::function<bool(const QString &)> &visible) {
    if (!node) return false;
    bool any=false;
    if (auto tabs=qobject_cast<QTabWidget *>(node)) {
        for (int i=0;i<tabs->count();++i) {
            auto page=tabs->widget(i); const bool shown=applyPanelLayoutVisibility(page,state,visible);
            state.tabVisibility(tabs,page,shown,false); any |= shown;
        }
    } else if (auto splitter=qobject_cast<QSplitter *>(node)) {
        for (int i=0;i<splitter->count();++i) any |= applyPanelLayoutVisibility(splitter->widget(i),state,visible);
    } else any=visible(node->property("groupPanel").toString());
    // QTabWidget controls its direct stacked page. Descendants of a covered
    // page must retain logical visibility so selecting that page reveals them.
    auto stack=qobject_cast<QStackedWidget *>(node->parentWidget());
    if (stack && qobject_cast<QTabWidget *>(stack->parentWidget()))
        node->setVisible(any && stack->currentWidget()==node);
    else node->setVisible(any);
    return any;
}
}
