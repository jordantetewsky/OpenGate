// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QWidget>
#include <QEvent>
#include "diagnostics.h"
namespace Unbound {
// The original widget keeps its layout slot/identity, but an empty transferred
// content container must not remain visibly occupying that slot.
class VacatedContents : public QObject {
public:
    ~VacatedContents() { restore(); }
    void hide(QWidget *slot) {
        restore(); original=slot;
        if (original) {
            wasHidden=original->isHidden(); minimum=original->minimumSize(); policy=original->sizePolicy();
            original->setMinimumSize(0,0); original->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored); original->hide();
            original->installEventFilter(this);
        }
    }
    void restore() {
        if (original) { original->removeEventFilter(this); original->setSizePolicy(policy); original->setMinimumSize(minimum); original->setHidden(wasHidden); }
        original=nullptr;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        // Resolve may re-show its native container during a later layout/toggle
        // update. Its contents are still leased; the empty slot must stay closed.
        // Remove this filter before docking so normal native visibility resumes.
        if (object==original && (event->type()==QEvent::Show || event->type()==QEvent::ShowToParent) && !original->isHidden()) {
            logEvent("workspace.native-slot.reshow-suppressed",{{"widget",original->objectName()}});
            original->hide();
        }
        return false;
    }
private:
    QPointer<QWidget> original; QSize minimum; QSizePolicy policy; bool wasHidden=false;
};
}
