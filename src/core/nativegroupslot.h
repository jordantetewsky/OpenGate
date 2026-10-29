// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>
#include "diagnostics.h"
namespace Unbound {
// A reversible native layout position. Empty slots have no painted placeholder
// or intrinsic footprint; an embedded arrangement supplies its real size hints.
class NativePanelSlot : public QWidget {
public:
    explicit NativePanelSlot(QWidget *parent=nullptr) : QWidget(parent) {
        setProperty("unboundNativeRestorationSlot",true);
        setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored); setMinimumSize(0,0);
    }
    QSize minimumSizeHint() const override { return layout() ? layout()->minimumSize() : QSize(0,0); }
    QSize sizeHint() const override { return layout() ? layout()->sizeHint() : QSize(0,0); }
};

// Own only the layout placed inside our restoration slot. Never wrap, resize or
// decorate an OS window; native content/session ownership stays with the caller.
class NativeGroupSlot {
public:
    NativeGroupSlot()=default;
    NativeGroupSlot(const NativeGroupSlot &)=delete;
    NativeGroupSlot &operator=(const NativeGroupSlot &)=delete;
    ~NativeGroupSlot() { release(); }
    bool active() const { return slot && surface && ownedLayout; }
    QWidget *marker() const { return slot; }
    QString mount(QWidget *marker,QWidget *embedded,QWidget *nativeOwner) {
        if (slot || !marker || !embedded || !nativeOwner || marker==embedded || embedded->isAncestorOf(marker) || embedded->isWindow() ||
            !marker->property("unboundNativeRestorationSlot").toBool() ||
            marker->window()!=nativeOwner || marker->layout())
            return QStringLiteral("The native arrangement slot is not available.");
        slot=marker; surface=embedded; owner=nativeOwner;
        minimum=slot->minimumSize(); policy=slot->sizePolicy(); hidden=slot->isHidden();
        ownedLayout=new QVBoxLayout(slot); ownedLayout->setContentsMargins(0,0,0,0); ownedLayout->setSpacing(0);
        ownedLayout->addWidget(surface);
        if (slot->objectName().startsWith("unboundNativePanelPosition_")) surface->setProperty("unboundScopedNativeArrangement",true);
        slot->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred); slot->setMinimumSize(0,0);
        surface->show(); slot->show(); slot->updateGeometry();
        logEvent("workspace.native-surface.mount",{{"slot",slot->objectName()},{"surface",surface->objectName()}});
        return {};
    }
    void setVisible(bool visible) { if (slot) { slot->setVisible(visible); slot->updateGeometry(); } }
    void release() {
        if (!slot && !surface && !ownedLayout) return;
        const bool sameLayout=slot && slot->layout()==ownedLayout;
        if (surface && slot && owner && slot->isAncestorOf(surface)) {
            if (ownedLayout) ownedLayout->removeWidget(surface);
            surface->setParent(owner); surface->hide();
            surface->setProperty("unboundScopedNativeArrangement",false);
        }
        if (ownedLayout) delete ownedLayout.data();
        if (slot) {
            slot->setSizePolicy(policy); slot->setMinimumSize(minimum); slot->setVisible(!hidden); slot->updateGeometry();
        }
        logEvent("workspace.native-surface.release",{{"slot",slot ? slot->objectName() : QString()},
            {"original_layout_retained",sameLayout}});
        slot=nullptr; surface=nullptr; owner=nullptr; ownedLayout=nullptr;
    }
private:
    QPointer<QWidget> slot,surface,owner;
    QPointer<QVBoxLayout> ownedLayout;
    QSize minimum; QSizePolicy policy; bool hidden=true;
};
}
