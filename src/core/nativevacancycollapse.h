// SPDX-License-Identifier: MIT
#pragma once
#include <QBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QPointer>
#include <QSet>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Collapse only native container branches made empty by leased panel slots.
// Do not relax the surviving controls, hide the whole main window, or transplant
// a second copy of Resolve's UI. Direct user-hidden state is never overridden.
class NativeVacancyCollapse {
public:
    ~NativeVacancyCollapse() { restore(); }
    void update(QWidget *page,const QList<QWidget *> &vacancies) {
        QSet<QWidget *> candidates,vacated,empty;
        for (auto slot:vacancies) {
            if (!slot || !page || !page->isAncestorOf(slot) || !slot->isHidden()) continue;
            vacated.insert(slot);
            for (auto parent=slot->parentWidget();parent && parent!=page && page->isAncestorOf(parent) && supported(parent);parent=parent->parentWidget())
                candidates.insert(parent);
        }
        std::function<bool(QWidget *)> occupied=[&](QWidget *container) {
            for (auto child:container->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly)) {
                if (child->isWindow() || vacated.contains(child) || qobject_cast<QSplitterHandle *>(child)) continue;
                if (child->isHidden() && !retained(child)) continue;
                if (candidates.contains(child)) { if (occupied(child)) return true; }
                else return true;
            }
            return false;
        };
        for (auto candidate:candidates) if (!occupied(candidate)) empty.insert(candidate);
        // A previously collapsed branch may now contain a newly shown native
        // panel. Re-evaluate without treating that branch itself as evidence.
        for (int i=collapsed.size()-1;i>=0;--i) {
            auto widget=collapsed[i];
            if (!widget || !empty.contains(widget)) {
                if (widget) { widget->show(); logEvent("workspace.native-branch.restored",{{"widget",widget->objectName()}}); }
                collapsed.removeAt(i);
            }
        }
        for (auto widget:empty) if (!widget->isHidden()) {
            collapsed.append(widget); widget->hide();
            logEvent("workspace.native-branch.collapsed",{{"widget",widget->objectName()}});
        }
    }
    void restore() {
        for (auto widget:collapsed) if (widget) widget->show();
        collapsed.clear();
    }
private:
    QList<QPointer<QWidget>> collapsed;
    bool retained(QWidget *widget) const {
        for (auto entry:collapsed) if (entry==widget) return true;
        return false;
    }
    static bool supported(QWidget *widget) {
        // Unknown custom-painted native containers are not assumed empty.
        const auto name=QByteArray(widget->metaObject()->className());
        return (name=="QWidget" || name=="QFrame" || qobject_cast<QSplitter *>(widget) || qobject_cast<QStackedWidget *>(widget)) &&
            (qobject_cast<QSplitter *>(widget) || qobject_cast<QStackedWidget *>(widget) ||
             qobject_cast<QBoxLayout *>(widget->layout()) || qobject_cast<QGridLayout *>(widget->layout()));
    }
};
}
