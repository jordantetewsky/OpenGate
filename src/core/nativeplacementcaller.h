// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <functional>
namespace Unbound {
// Reuse an existing native placement button without invoking a second native
// floating-window implementation. Ownership and movement stay in panel sessions.
class NativePlacementCaller : public QObject {
public:
    NativePlacementCaller(QAbstractButton *button,QObject *owner):QObject(owner),button(button) {
        button->installEventFilter(this);
    }
    std::function<void()> activate;
    std::function<void(QPoint)> drag;
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if(!button)return false;
        if(event->type()==QEvent::MouseButtonPress) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(mouse->button()!=Qt::LeftButton)return false;
            press=mouse->globalPos();armed=true;moved=false;button->setDown(true);return true;
        }
        if(event->type()==QEvent::MouseMove && armed) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(drag && mouse->buttons().testFlag(Qt::LeftButton) && (mouse->globalPos()-press).manhattanLength()>=QApplication::startDragDistance()) {
                armed=false;moved=true;button->setDown(false);drag(mouse->globalPos());
            }
            return true;
        }
        if(event->type()==QEvent::MouseButtonRelease) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(mouse->button()!=Qt::LeftButton)return false;
            const bool invoke=armed&&!moved&&button->rect().contains(mouse->pos());armed=false;button->setDown(false);
            if(invoke&&activate)activate();return true;
        }
        if(event->type()==QEvent::KeyPress || event->type()==QEvent::KeyRelease) {
            auto key=static_cast<QKeyEvent *>(event);
            if(key->key()!=Qt::Key_Space && key->key()!=Qt::Key_Return)return false;
            if(event->type()==QEvent::KeyRelease&&!key->isAutoRepeat()&&activate)activate();return true;
        }
        if(event->type()==QEvent::Shortcut){if(activate)activate();return true;}
        if(event->type()==QEvent::Hide || event->type()==QEvent::UngrabMouse){armed=false;button->setDown(false);}
        return false;
    }
private:
    QPointer<QAbstractButton> button;QPoint press;bool armed=false,moved=false;
};
}
