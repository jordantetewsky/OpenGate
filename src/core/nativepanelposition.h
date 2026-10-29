// SPDX-License-Identifier: MIT
#pragma once
#include <QBoxLayout>
#include <QGridLayout>
#include <QPointer>
#include <QSplitter>
#include <QStackedWidget>
#include <functional>
#include "nativegroupslot.h"
namespace Unbound {
// Lease one native layout position, not the entire Edit page. The position owns
// its marker independently of panel sessions, so removing one member cannot
// delete the layout occupied by the remaining group.
class NativePanelPosition {
public:
    struct Destination {
        QWidget *parent=nullptr;
        QLayout *layout=nullptr;
        QSplitter *splitter=nullptr;
        QStackedWidget *stack=nullptr;
        QList<int> sizes;
        QRect geometry;
        Qt::WindowFlags flags;
        int stackIndex=-1;
        bool hidden=false;
    };
    NativePanelPosition()=default;
    NativePanelPosition(const NativePanelPosition &)=delete;
    NativePanelPosition &operator=(const NativePanelPosition &)=delete;
    ~NativePanelPosition() { restore(); }
    bool active() const { return marker && original && parent; }
    QWidget *surface() const { return marker; }
    static bool supported(QWidget *panel,QWidget *page) {
        if (!panel || !page || panel==page || !page->isAncestorOf(panel) || panel->isWindow() || !panel->isVisible()) return false;
        auto owner=panel->parentWidget();
        if (auto split=qobject_cast<QSplitter *>(owner)) return split->indexOf(panel)>=0;
        if (auto stack=qobject_cast<QStackedWidget *>(owner)) return stack->indexOf(panel)>=0;
        auto layout=owner->layout();
        return layout && (qobject_cast<QBoxLayout *>(layout) || qobject_cast<QGridLayout *>(layout)) && layout->indexOf(panel)>=0;
    }
    QString acquire(QWidget *panel,QWidget *page,bool allowParked=false) {
        bool parked=false;
        if(allowParked&&panel&&page)for(auto p=panel->parentWidget();p;p=p->parentWidget())
            if(p->objectName()=="unboundNativeLayoutStash"&&p->parentWidget()==page&&p->property("unboundParkedNativePage").value<QObject *>())parked=true;
        if (active() || !panel || !page || panel==page || !page->isAncestorOf(panel) || panel->isWindow() || (!panel->isVisible()&&!parked))
            return QStringLiteral("The native panel position is unavailable.");
        auto owner=panel->parentWidget();
        auto split=qobject_cast<QSplitter *>(owner); auto stack=qobject_cast<QStackedWidget *>(owner);
        auto candidate=split || stack ? nullptr : owner->layout();
        if ((!split && !stack && !qobject_cast<QBoxLayout *>(candidate) && !qobject_cast<QGridLayout *>(candidate)) ||
            (candidate && candidate->indexOf(panel)<0) || (split && split->indexOf(panel)<0) || (stack && stack->indexOf(panel)<0))
            return QStringLiteral("This native panel position is not supported.");
        logEvent("workspace.native-position.acquire.request",{{"panel",panel->objectName()},{"parent",owner->objectName()},
            {"layout",candidate ? candidate->metaObject()->className() : owner->metaObject()->className()}});
        original=panel; parent=owner; nativePage=page; layout=candidate; splitter=split; stacked=stack;
        hidden=panel->isHidden(); geometry=panel->geometry(); flags=panel->windowFlags();
        if (splitter) sizes=splitter->sizes();
        if (stacked) { index=stacked->indexOf(panel); selected=stacked->currentWidget(); }
        // Never construct a marker directly in QSplitter: it inserts children.
        marker=new NativePanelSlot;
        marker->setObjectName("unboundNativePanelPosition_"+panel->objectName());
        marker->setSizePolicy(panel->sizePolicy()); marker->setMinimumSize(0,0);
        bool replaced=false;
        if (stacked) {
            stacked->removeWidget(panel); stacked->insertWidget(index,marker);
            if (selected==panel) stacked->setCurrentWidget(marker);
            replaced=true;
        } else if (splitter) replaced=splitter->replaceWidget(splitter->indexOf(panel),marker)==panel;
        else if (auto item=layout->replaceWidget(panel,marker,Qt::FindDirectChildrenOnly)) { delete item; replaced=true; }
        if (!replaced) { delete marker.data(); clear(); return QStringLiteral("Could not preserve the native panel position."); }
        parking=new QWidget(page); parking->setObjectName("unboundNativePanelPositionParking"); parking->hide();
        panel->setParent(parking); panel->hide(); marker->show();
        if (splitter) splitter->setSizes(sizes);
        logEvent("workspace.native-position.acquire",{{"panel",panel->objectName()},{"parent",parent->objectName()}});
        return {};
    }
    // The caller must return leased content before restoring its native slot.
    // Unexpected replacement leaves both trees intact for recovery.
    bool restore() {
        if (!marker && !original) { clear(); return true; }
        if (!active() || !parking || original->parentWidget()!=parking || marker->layout()) return rejected();
        bool restored=false;
        if (stacked && marker->parentWidget()==stacked && stacked->indexOf(marker)>=0) {
            const bool wasSelected=stacked->currentWidget()==marker;
            const int currentIndex=stacked->indexOf(marker);
            stacked->removeWidget(marker); stacked->insertWidget(currentIndex,original);
            if (wasSelected) stacked->setCurrentWidget(original);
            restored=true;
        } else if (splitter && marker->parentWidget()==splitter && splitter->indexOf(marker)>=0)
            restored=splitter->replaceWidget(splitter->indexOf(marker),original)==marker;
        else if (layout && parent->layout()==layout && layout->indexOf(marker)>=0)
            if (auto item=layout->replaceWidget(marker,original,Qt::FindDirectChildrenOnly)) { delete item; restored=true; }
        if (!restored) return rejected();
        original->setParent(parent,flags); original->setGeometry(geometry); original->setVisible(!hidden && (!stacked || stacked->currentWidget()==original));
        if (splitter) splitter->setSizes(sizes);
        logEvent("workspace.native-position.restore",{{"panel",original->objectName()}});
        delete marker.data(); delete parking.data(); clear(); return true;
    }
    QWidget *content() const { return original; }
    QWidget *contentParking() const { return parking; }
    // Return the native position to a still-floating session's restoration
    // marker. Rebind its lifetime observer before deleting our parking owner.
    bool handoff(QWidget *replacement,const std::function<void(const Destination &)> &rebind) {
        if (!active() || !parking || !replacement || replacement==original || !rebind || marker->layout() ||
            replacement->parentWidget()!=parking || !replacement->property("unboundNativeRestorationSlot").toBool() ||
            parking->isAncestorOf(original)) return rejected();
        bool replaced=false;
        if (stacked && marker->parentWidget()==stacked && stacked->indexOf(marker)>=0) {
            const bool current=stacked->currentWidget()==marker; index=stacked->indexOf(marker);
            stacked->removeWidget(marker); stacked->insertWidget(index,replacement);
            if (current) stacked->setCurrentWidget(replacement);
            replaced=true;
        } else if (splitter && marker->parentWidget()==splitter && splitter->indexOf(marker)>=0)
            replaced=splitter->replaceWidget(splitter->indexOf(marker),replacement)==marker;
        else if (layout && parent->layout()==layout && layout->indexOf(marker)>=0)
            if (auto item=layout->replaceWidget(marker,replacement,Qt::FindDirectChildrenOnly)) { delete item; replaced=true; }
        if (!replaced) return rejected();
        replacement->setParent(parent);
        rebind({parent,layout,splitter,stacked,sizes,geometry,flags,index,hidden});
        replacement->hide();
        logEvent("workspace.native-position.handoff",{{"panel",original->objectName()},{"parent",parent->objectName()}});
        delete marker.data(); delete parking.data(); clear(); return true;
    }
private:
    bool rejected() const { logEvent("workspace.native-position.restore-rejected",{{"reason","position-or-content-ownership-changed"}}); return false; }
    void clear() { marker=nullptr; original=nullptr; parent=nullptr; parking=nullptr; nativePage=nullptr; layout=nullptr; splitter=nullptr; stacked=nullptr; selected=nullptr; sizes.clear(); }
    QPointer<QWidget> marker,original,parent,parking,nativePage,selected;
    QPointer<QLayout> layout;
    QPointer<QSplitter> splitter;
    QPointer<QStackedWidget> stacked;
    QList<int> sizes;
    QRect geometry; Qt::WindowFlags flags; int index=-1; bool hidden=false;
};
}
