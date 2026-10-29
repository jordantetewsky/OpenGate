// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractItemView>
#include <QSet>
#include <QJsonArray>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Inspect command names only, never preset contents or unrelated user data.
inline void inspectNativeKeyboardCommands(QWidget *dialog) {
    QSet<QAbstractItemModel *> seen; QSet<QString> names; int visited=0; bool truncated=false;
    std::function<void(QAbstractItemModel *,QModelIndex,int)> walk;
    walk=[&](QAbstractItemModel *model,QModelIndex parent,int depth) {
        if (depth>16) { truncated=true; return; }
        for (int row=0;row<model->rowCount(parent);++row) {
            if (++visited>30000) { truncated=true; return; }
            const auto index=model->index(row,0,parent); if (!index.isValid()) continue;
            const auto name=index.data(Qt::DisplayRole).toString();
            if (name.startsWith("OpenGate - ")) names.insert(name);
            walk(model,index,depth+1);
        }
    };
    for (auto view:dialog->findChildren<QAbstractItemView *>()) if (view->model() && !seen.contains(view->model())) {
        seen.insert(view->model()); walk(view->model(),view->rootIndex(),0);
    }
    QJsonArray commands; auto sorted=names.values(); sorted.sort(); for (const auto &name:sorted) commands.append(name);
    logEvent("keyboard.native.command-list",{{"commands",commands},{"count",commands.size()},
        {"visited_rows",visited},{"truncated",truncated},{"scope","displayed native models; no assignment writes"}});
}
}
