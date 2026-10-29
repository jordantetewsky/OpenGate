// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Retain the current Edit workspace; never load a named preset on page return.
// The owner provides suspend/resume presentation, not destructive dock/undock.
class EditPageLifecycle : public QObject {
public:
    explicit EditPageLifecycle(QObject *owner=nullptr) : QObject(owner) {}
    std::function<bool()> running=[] { return true; };
    std::function<void()> checkpoint,suspend,resume;
    void observe(QWidget *edit) {
        if (page==edit) return;
        if (page) page->removeEventFilter(this);
        page=edit; ++generation; active=false;
        if (!page) return;
        page->installEventFilter(this);
        if (page->isVisible()) enter();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (object!=page || !running()) return false;
        if (event->type()==QEvent::Hide && active) {
            active=false; const auto request=++generation;
            logEvent("workspace.edit.suspend.request",{{"deferred",true}});
            // Hide is delivered while Qt can still be walking native children.
            // Never reparent the footer or restore layouts inside that traversal.
            QTimer::singleShot(0,this,[this,request] {
                if (request!=generation || active || !page || page->isVisible() || !running()) return;
                logEvent("workspace.edit.suspend.begin");
                if (checkpoint) checkpoint();
                if (request!=generation || active || !page || page->isVisible() || !running()) return;
                if (suspend) suspend();
                logEvent("workspace.edit.suspended",{{"retained",true},{"preset_reloaded",false}});
            });
        }
        if (event->type()==QEvent::Show && !active) enter();
        return false;
    }
private:
    QPointer<QWidget> page; bool active=false; unsigned generation=0;
    void enter() {
        active=true; const auto request=++generation;
        QTimer::singleShot(0,this,[this,request] {
            if (request!=generation || !active || !page || !page->isVisible() || !running()) return;
            if (resume) resume();
            logEvent("workspace.edit.resumed",{{"retained",true},{"preset_reloaded",false}});
        });
    }
};
}
