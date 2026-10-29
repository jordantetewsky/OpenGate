// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSplitter>
#include <QTabWidget>
namespace Unbound {
inline QJsonObject panelLayoutSnapshot(QWidget *widget) {
    if (!widget) return {};
    if (auto tabs=qobject_cast<QTabWidget *>(widget)) {
        QJsonArray children;
        for (int i=0;i<tabs->count();++i) children.append(panelLayoutSnapshot(tabs->widget(i)));
        return {{"orientation","tabs"},{"children",children},{"selected",tabs->currentIndex()}};
    }
    if (auto splitter=qobject_cast<QSplitter *>(widget)) {
        QJsonArray children,sizes;
        for (int i=0;i<splitter->count();++i) children.append(panelLayoutSnapshot(splitter->widget(i)));
        auto measured=splitter->sizes();
        const auto remembered=splitter->property("unboundVisibleSizes").value<QList<int>>();
        if (measured.contains(0) && remembered.size()==measured.size()) measured=remembered;
        for (int size : measured) sizes.append(qMax(1,size));
        return {{"orientation",splitter->orientation()==Qt::Horizontal ? "horizontal" : "vertical"},
            {"children",children},{"sizes",sizes}};
    }
    return {{"panel",widget->property("groupPanel").toString()}};
}
}
