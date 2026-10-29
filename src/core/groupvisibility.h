// SPDX-License-Identifier: MIT
#pragma once
#include <QHash>
#include <QJsonArray>
#include <QMap>
#include <QPointer>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include "diagnostics.h"
namespace Unbound {
inline bool panelCoveredByTab(QWidget *panel) {
    for (auto child=panel; child && child->parentWidget(); child=child->parentWidget())
        if (auto stack=qobject_cast<QStackedWidget *>(child->parentWidget()))
            if (qobject_cast<QTabWidget *>(stack->parentWidget()) && stack->currentWidget()!=child) return true;
    return false;
}
// Remember divider sizes for each open/closed combination, rather than rescaling repeatedly.
class GroupVisibility
{
public:
    void reset() { splitters.clear(); returnTabs.clear(); }
    void selectPath(QWidget *leaf,QWidget *boundary)
    {
        const auto path=tabPath(leaf,boundary);
        QJsonArray transitions;
        for (const auto &step : path) transitions.append(QJsonObject{{"from",step.first->currentIndex()},{"to",step.first->indexOf(step.second)}});
        for (int i=path.size()-1;i>=0;--i) tabVisibility(path[i].first,path[i].second,true,true);
        logEvent("panel.tab-path.select",{{"panel",leaf ? leaf->property("groupPanel").toString() : QString()},
            {"levels",path.size()},{"transitions",transitions}});
    }
    void closePath(QWidget *leaf,QWidget *boundary)
    {
        const auto path=tabPath(leaf,boundary);
        // Hiding a covered tab must not change the user's current selection.
        for (const auto &step : path) if (step.first->currentWidget()!=step.second) {
            logEvent("panel.tab-path.covered",{{"panel",leaf ? leaf->property("groupPanel").toString() : QString()},{"levels",path.size()}});
            return;
        }
        QJsonArray transitions;
        for (const auto &step : path) {
            const int before=step.first->currentIndex();
            auto prior=returnTabs.take(step.first);
            if (prior && step.first->indexOf(prior)>=0 && step.first->isTabVisible(step.first->indexOf(prior)))
                step.first->setCurrentWidget(prior);
            transitions.append(QJsonObject{{"from",before},{"to",step.first->currentIndex()}});
        }
        logEvent("panel.tab-path.restore",{{"panel",leaf ? leaf->property("groupPanel").toString() : QString()},
            {"levels",path.size()},{"transitions",transitions}});
    }
    void remember(QWidget *root)
    {
        auto items = root->findChildren<QSplitter *>();
        if (auto splitter = qobject_cast<QSplitter *>(root)) items.prepend(splitter);
        for (auto splitter : items) {
            const auto sizes=splitter->sizes(); splitters[splitter][mask(splitter)]=sizes;
            if (sizes.size()==splitter->count() && !sizes.contains(0)) splitter->setProperty("unboundVisibleSizes",QVariant::fromValue(sizes));
        }
    }
    void restore(QWidget *root)
    {
        auto items = root->findChildren<QSplitter *>();
        if (auto splitter = qobject_cast<QSplitter *>(root)) items.prepend(splitter);
        for (auto splitter : items) {
            splitter->refresh();
            const auto sizes = splitters.value(splitter).value(mask(splitter));
            if (sizes.size() == splitter->count()) splitter->setSizes(sizes);
        }
    }
    void tabVisibility(QTabWidget *tabs, QWidget *panel, bool visible, bool select)
    {
        const int index = tabs->indexOf(panel); if (index < 0) return;
        if (visible && select && tabs->currentWidget() != panel) returnTabs[tabs] = tabs->currentWidget();
        const bool closingSelected = !visible && tabs->currentWidget() == panel;
        tabs->setTabVisible(index, visible);
        if (visible && select) tabs->setCurrentWidget(panel);
        if (closingSelected) {
            auto prior = returnTabs.take(tabs);
            if (prior && tabs->indexOf(prior) >= 0 && tabs->isTabVisible(tabs->indexOf(prior))) tabs->setCurrentWidget(prior);
        }
    }
private:
    static QList<QPair<QTabWidget *,QWidget *>> tabPath(QWidget *leaf,QWidget *boundary)
    {
        QList<QPair<QTabWidget *,QWidget *>> result;
        for (auto child=leaf; child && child->parentWidget() && child->parentWidget()!=boundary; child=child->parentWidget())
            if (auto stack=qobject_cast<QStackedWidget *>(child->parentWidget()))
                if (auto tabs=qobject_cast<QTabWidget *>(stack->parentWidget()))
                    if (tabs->indexOf(child)>=0) result.append({tabs,child});
        return result;
    }
    static QString mask(QSplitter *splitter)
    {
        QString result;
        for (int i = 0; i < splitter->count(); ++i) result += splitter->widget(i)->isHidden() ? QLatin1Char('0') : QLatin1Char('1');
        return result;
    }
    QHash<QSplitter *, QMap<QString, QList<int>>> splitters;
    QHash<QTabWidget *, QPointer<QWidget>> returnTabs;
};
}
