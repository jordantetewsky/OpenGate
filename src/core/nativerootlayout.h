// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QPointer>
#include <QVBoxLayout>
#include <QEvent>
#include <QRegion>
#include "diagnostics.h"
#include "../platform/workspacewindowintegration.h"
namespace Unbound {
// Lease a page's layout as one unit, keeping its native slot graph intact for
// restoration. Only a custom arrangement occupies the page while the lease is
// active. This does not create, decorate or hide the main native window.
class NativeRootLayout : public QObject {
public:
    explicit NativeRootLayout(QObject *owner=nullptr) : QObject(owner) {}
    ~NativeRootLayout() override { restore(); }
    bool active() const { return page && nativeLayout && customLayout; }
    static bool ownsParkedWidget(QWidget *widget,QWidget *nativePage) {
        if(!widget || !nativePage || widget==nativePage)return false;
        for(auto parent=widget->parentWidget();parent;parent=parent->parentWidget())
            if(parent->objectName()=="unboundNativeLayoutStash" && parent->parentWidget()==nativePage->window() &&
               parent->property("unboundParkedNativePage").value<QObject *>()==nativePage && parent->layout())return true;
        return false;
    }
    static bool contains(QWidget *nativePage,QWidget *container) {
        if (!nativePage || !container || container==nativePage) return false;
        for (auto ancestor=container; ancestor && ancestor!=nativePage; ancestor=ancestor->parentWidget()) {
            auto marker=ancestor->parentWidget();
            if (ancestor->property("unboundScopedNativeArrangement").toBool() && marker &&
                marker->objectName().startsWith("unboundNativePanelPosition_") && marker->layout() &&
                marker->layout()->indexOf(ancestor)>=0 && nativePage->isAncestorOf(marker)) return true;
            if (ancestor->parentWidget()==nativePage && ancestor->property("unboundNativeArrangementRoot").toBool())
                return nativePage->layout() && nativePage->layout()->indexOf(ancestor)>=0;
        }
        return false;
    }
    static QWidget *restorationParent(QWidget *nativePage,QWidget *originalParent,QLayout *originalLayout) {
        if (!nativePage || !originalParent || originalParent!=nativePage || !originalLayout || originalParent->layout()==originalLayout) return originalParent;
        auto retained=originalLayout->parentWidget();
        return retained && retained->layout()==originalLayout &&
            retained->property("unboundNativeLayoutRemainder").toBool() && contains(nativePage,retained)
            ? retained : originalParent;
    }
    static QWidget *logicalParent(QWidget *nativePage,QWidget *currentParent) {
        return currentParent && ((currentParent->property("unboundNativeLayoutRemainder").toBool() && contains(nativePage,currentParent)) || (currentParent->objectName()=="unboundNativeLayoutStash" && currentParent->property("unboundParkedNativePage").value<QObject *>()==nativePage))
            ? nativePage : currentParent;
    }
    QString mount(QWidget *nativePage,QWidget *arrangement) {
        if (page || !nativePage || !nativePage->layout() || !arrangement || arrangement->isWindow() ||
            arrangement==nativePage || arrangement->isAncestorOf(nativePage))
            return QStringLiteral("The native workspace layout is not available.");
        page=nativePage; surface=arrangement; main=page->window(); nativeLayout=page->layout();
        pageDestroyed=connect(page,&QObject::destroyed,this,[this] {
            if (stash) stash->deleteLater();
            clear(); logEvent("workspace.native-root.owner-destroyed");
        });
        minimum=page->minimumSize(); policy=page->sizePolicy();
        for (auto child : page->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
            if (child!=surface && !child->isWindow()) baselineVisibility.append({child,child->isHidden(),child->geometry()});
        const auto margins=nativeLayout->contentsMargins();
        stash=new QWidget(main); stash->setObjectName("unboundNativeLayoutStash"); stash->hide();
        stash->setProperty("unboundParkedNativePage",QVariant::fromValue<QObject *>(page));
        stash->setLayout(nativeLayout);
        if (page->layout() || stash->layout()!=nativeLayout) {
            if (!page->layout() && stash->layout()==nativeLayout) page->setLayout(nativeLayout);
            // A native callback may have installed a different page layout.
            // Retain our parked native tree rather than deleting its controls.
            if (page->layout()==nativeLayout) { delete stash.data(); clear(); }
            else logEvent("workspace.native-root.mount-rejected",{{"reason","native-layout-changed-during-transfer"}});
            return QStringLiteral("Could not preserve the native workspace layout.");
        }
        // Preserve visibility of unmanaged direct children too; native custom
        // widgets are not necessarily represented by a QLayoutItem.
        for (auto child : page->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
            if (child!=surface && !child->isWindow()) { visibility.append({child,child->isHidden(),child->geometry()}); child->hide(); }
        auto safeMargins=margins;safeMargins.setTop(qMax(margins.top(),nativeWorkspaceTopInset(page)));
        customLayout=new QVBoxLayout(page); customLayout->setContentsMargins(safeMargins); customLayout->setSpacing(0);
        customLayout->addWidget(surface);
        rootProperty=surface->property("unboundNativeArrangementRoot");
        surface->setProperty("unboundNativeArrangementRoot",true);
        page->setMinimumSize(0,0); surface->show();
        logEvent("workspace.native-root.mount",{{"page",page->objectName()},{"native_layout",nativeLayout->metaObject()->className()}});
        return {};
    }
    // Keep the still-native layout as a real leaf of the arrangement. This is
    // deliberately explicit: adopting only some panes must not hide all others.
    QString retainNativeRemainder(QWidget *container) {
        if (!active() || remainder || !container || container->layout() ||
            !contains(page,container) || container==surface || stash->isAncestorOf(container))
            return QStringLiteral("The remaining native workspace cannot be placed here.");
        releaseNativeVisibility();
        remainder=container;
        auto layout=new QVBoxLayout(container); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
        layout->addWidget(stash);
        stash->setProperty("unboundNativeLayoutRemainder",true);
        for (const auto &item : visibility) if (item.widget && item.widget->parentWidget()==page) {
            item.widget->setParent(stash); item.widget->setGeometry(item.geometry); item.widget->setVisible(!item.hidden);
        }
        stash->show();
        logEvent("workspace.native-root.remainder",{{"page",page->objectName()},{"unmanaged_controls",visibility.size()}});
        return {};
    }
    // Resolve's Edit loading route needs the parked native tree effectively
    // visible. Preserve that state without adding a layout leaf, pixels or
    // pointer targets; individually hidden children stay hidden.
    void preserveNativeVisibility() {
        if (!active() || remainder || nativeVisibilityPreserved ||
            page->objectName()!="WStackPage_Conform") return;
        nativeVisibilityPreserved=true;
        stash->setAttribute(Qt::WA_TransparentForMouseEvents,true);
        stash->setAttribute(Qt::WA_ShowWithoutActivating,true);
        // An empty mask means 'no mask' in Qt. Use a nonempty region wholly
        // outside the widget instead, so even native background stays clipped.
        stash->setMask(QRegion(-1,-1,1,1));
        page->installEventFilter(this);
        stash->setVisible(page->isVisible());
        logEvent("workspace.native-root.visibility-preserved",{{"enabled",true},
            {"stash_visible",stash->isVisible()},{"mask_intersects_rect",stash->mask().intersects(stash->rect())}});
    }
    bool restore() {
        if (!page) return true;
        if (!nativeLayout || !stash || stash->layout()!=nativeLayout || page->layout()!=customLayout) {
            logEvent("workspace.native-root.restore-rejected",{{"reason","native-or-custom-layout-replaced"}});
            return false;
        }
        releaseNativeVisibility();
        // Park our tree before returning the original layout. Never delete a
        // native control merely because its presentation container is retiring.
        auto parking=new QWidget(main); parking->hide(); parking->setLayout(customLayout);
        auto nativeVisibility=baselineVisibility;
        if (remainder) {
            nativeVisibility.clear();
            for (auto child : stash->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
                if (!child->isWindow()) nativeVisibility.append({child,child->isHidden(),child->geometry()});
        }
        page->setLayout(nativeLayout);
        if (page->layout()!=nativeLayout) {
            if (!page->layout()) page->setLayout(customLayout);
            if (surface && parking->isAncestorOf(surface)) {
                if (parking->layout()) parking->layout()->removeWidget(surface);
                surface->setProperty("unboundNativeArrangementRoot",rootProperty);
                surface->setParent(main); surface->hide();
            }
            delete parking;
            logEvent("workspace.native-root.restore-rejected",{{"reason","native-layout-transfer-failed"}});
            return false;
        }
        if (surface) {
            if (parking->layout()) parking->layout()->removeWidget(surface);
            surface->setProperty("unboundNativeArrangementRoot",rootProperty);
            surface->setParent(main); surface->hide();
        }
        page->setMinimumSize(minimum); page->setSizePolicy(policy);
        // QLayout defers showing reparented children. Apply logical visibility
        // now so an immediate command can find a just-restored native panel.
        for (const auto &item : nativeVisibility)
            if (item.widget && item.widget->parentWidget()==page) item.widget->setVisible(!item.hidden);
        for (const auto &item : visibility)
            if (item.widget) {
                if (item.widget->parentWidget()==stash) { item.widget->setParent(page); item.widget->setGeometry(item.geometry); }
                if (item.widget->parentWidget()==page) item.widget->setVisible(!item.hidden);
            }
        disconnect(pageDestroyed);
        delete parking; delete stash.data();
        logEvent("workspace.native-root.restore",{{"page",page->objectName()},{"native_layout_restored",true}});
        clear(); return true;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (nativeVisibilityPreserved && object==page && stash) {
            if (event->type()==QEvent::Hide) stash->hide();
            else if (event->type()==QEvent::Show) stash->show();
        }
        return false;
    }
private:
    void releaseNativeVisibility() {
        if (!nativeVisibilityPreserved) return;
        if (page) page->removeEventFilter(this);
        if (stash) {
            stash->hide(); stash->clearMask();
            stash->setAttribute(Qt::WA_TransparentForMouseEvents,false);
            stash->setAttribute(Qt::WA_ShowWithoutActivating,false);
        }
        nativeVisibilityPreserved=false;
    }
    void clear() {
        if (page && nativeVisibilityPreserved) page->removeEventFilter(this);
        nativeVisibilityPreserved=false;
        disconnect(pageDestroyed); pageDestroyed={};
        page=nullptr; main=nullptr; surface=nullptr; stash=nullptr; remainder=nullptr; nativeLayout=nullptr; customLayout=nullptr;
        visibility.clear(); baselineVisibility.clear();
    }
    QPointer<QWidget> page,main,surface,stash,remainder;
    QPointer<QLayout> nativeLayout,customLayout;
    struct NativeVisibility { QPointer<QWidget> widget; bool hidden; QRect geometry; };
    QList<NativeVisibility> visibility,baselineVisibility;
    QMetaObject::Connection pageDestroyed;
    QSize minimum; QSizePolicy policy;
    QVariant rootProperty;
    bool nativeVisibilityPreserved=false;
};
}
