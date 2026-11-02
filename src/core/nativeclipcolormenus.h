// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <functional>
#include "menuoverflowscroll.h"
namespace Unbound {
// Integrate native clip-color menus, retaining the originating widget across
// nested popup focus changes. No clip lookup, painting or command execution.
class NativeClipColorMenus : public QObject {
public:
    using Prepare=std::function<void(QMenu *,QWidget *)>;
    NativeClipColorMenus(QObject *owner,Prepare prepare):QObject(owner),prepare(std::move(prepare)) {
        origin=QApplication::focusWidget();qApp->installEventFilter(this);
        connect(qApp,&QApplication::focusChanged,this,[this](QWidget *,QWidget *next){
            if(next && !qobject_cast<QMenu *>(next) && !QApplication::activePopupWidget())origin=next;
        });
    }
    ~NativeClipColorMenus() override {if(qApp)qApp->removeEventFilter(this);}
    static bool matches(QMenu *menu) {
        if(!menu)return false;
        if(menu->objectName()=="markSetClipColor" || label(menu->menuAction()->text())=="clip color")return true;
        QSet<QString> names;for(auto action:menu->actions())names.insert(label(action->text()));
        return names.contains("orange") && names.contains("apricot") && names.contains("chocolate");
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::ContextMenu)if(auto widget=qobject_cast<QWidget *>(object))
            if(!qobject_cast<QMenu *>(widget))origin=widget;
        if(event->type()!=QEvent::Show)return false;
        auto menu=qobject_cast<QMenu *>(object);if(!menu)return false;
        if(!watched.contains(menu)) {
            watched.insert(menu);connect(menu,&QObject::destroyed,this,[this,menu]{watched.remove(menu);});
            connect(menu,&QMenu::aboutToShow,this,[this,menu]{refresh(menu);});
        }
        refresh(menu);
        // Native context menus can populate their actions in their Show handler.
        QPointer<QMenu> alive=menu;
        QTimer::singleShot(0,this,[this,alive]{if(alive && alive->isVisible())refresh(alive);});
        return false;
    }
private:
    QPointer<QWidget> origin;QSet<QMenu *> watched;Prepare prepare;
    static QString label(QString text){text.remove('&');return text.trimmed().toLower();}
    void refresh(QMenu *menu){if(matches(menu) && prepare){prepare(menu,origin);MenuOverflowScroll::refresh(menu);}}
};
}
