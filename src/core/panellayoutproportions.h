// SPDX-License-Identifier: MIT
#pragma once
#include <QSplitter>
#include <QVariant>

namespace Unbound {
// Apply saved proportions in the space actually allocated to each splitter.
// Recipes use small weights; snapshots use pixel measurements. Both describe
// proportions, but passing weights directly to setSizes clamps them to unrelated
// child minima before Qt distributes the available space.
inline void restorePanelLayoutProportions(QWidget *root)
{
    if (!root) return;
    auto splitters = root->findChildren<QSplitter *>();
    if (auto splitter = qobject_cast<QSplitter *>(root)) splitters.prepend(splitter);
    // Parents first: their allocation determines the nested splitter's extent.
    for (auto splitter : splitters) {
        const auto saved = splitter->property("savedSizes").toList();
        if (saved.size() != splitter->count()) continue;
        qint64 total = 0;
        for (const auto &size : saved) total += qMax(1, size.toInt());
        const auto rect = splitter->contentsRect();
        const int extent = splitter->orientation() == Qt::Horizontal ? rect.width() : rect.height();
        int shown = 0;
        for (int i = 0; i < splitter->count(); ++i)
            if (!splitter->widget(i)->isHidden()) ++shown;
        const int available = qMax(1, extent - qMax(0, shown - 1) * splitter->handleWidth());
        QList<int> sizes;
        qint64 cumulative = 0;
        int assigned = 0;
        for (const auto &size : saved) {
            cumulative += qMax(1, size.toInt());
            const int end = int(cumulative * available / total);
            sizes.append(qMax(1, end - assigned));
            assigned = end;
        }
        splitter->setSizes(sizes);
    }
}
}
