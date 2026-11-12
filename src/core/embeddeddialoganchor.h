// SPDX-License-Identifier: MIT
#pragma once
#include <QDialog>
#include <QEvent>
#include <QLayout>
#include <QPointer>

namespace Unbound {
// Retains layout ownership of an embedded native dialog's position. Its native
// header may still call move(), but only the outer host can be moved by dragging.
// No mouse/key events are swallowed: preset controls and native Save stay intact.
class EmbeddedDialogAnchor : public QObject {
public:
    explicit EmbeddedDialogAnchor(QDialog *dialog):QObject(dialog),dialog(dialog) {
        dialog->installEventFilter(this);bindParent();anchor();
    }
    ~EmbeddedDialogAnchor() override {
        if(parent)parent->removeEventFilter(this);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==dialog && event->type()==QEvent::ParentChange)bindParent();
        if((object==dialog && (event->type()==QEvent::Move || event->type()==QEvent::Show ||
                              event->type()==QEvent::ParentChange)) ||
           (object==parent && (event->type()==QEvent::Resize || event->type()==QEvent::LayoutRequest)))anchor();
        return false;
    }
private:
    QPointer<QDialog> dialog;
    QPointer<QWidget> parent;
    bool correcting=false;
    void bindParent() {
        if(parent)parent->removeEventFilter(this);
        parent=dialog?dialog->parentWidget():nullptr;
        if(parent)parent->installEventFilter(this);
    }
    void anchor() {
        if(correcting || !dialog || dialog->isWindow() || !parent || !parent->layout() ||
           parent->layout()->indexOf(dialog)<0)return;
        const auto position=parent->layout()->contentsRect().topLeft();
        if(dialog->pos()==position)return;
        correcting=true;dialog->move(position);correcting=false;
    }
};
}
