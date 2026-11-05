// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAbstractButton>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Inspect the real selector's public Qt state. Never supply, refresh, reparent,
// activate or replace a native menu, or consume its input.
class NativeProjectSelectorInspection : public QObject {
public:
    explicit NativeProjectSelectorInspection(QObject *owner) : QObject(owner) {
        qApp->installEventFilter(this);
    }
    ~NativeProjectSelectorInspection() override { qApp->removeEventFilter(this); }
    void observe(QAbstractButton *button) {
        if (!button || buttons.contains(button)) return;
        buttons.insert(button);
        connect(button,&QObject::destroyed,this,[this,button]{buttons.remove(button);});
        record(button,"discovery");
    }
    static QJsonObject capture(QAbstractButton *button) {
        QJsonObject result{{"read_only",true},{"exists",button!=nullptr}};
        if (!button) return result;
        result["button"]=widgetState(button,false);
        result["signals_blocked"]=button->signalsBlocked();
        result["native_id_ephemeral"]=QString::number(quintptr(button),16);
        result["window"]=widgetState(button->window(),false);
        result["active_window"]=widgetState(QApplication::activeWindow(),false);
        result["mouse_grabber"]=widgetState(QWidget::mouseGrabber(),false);
        result["embedded"]=button->graphicsProxyWidget()!=nullptr;
        auto push=qobject_cast<QPushButton *>(button);
        auto menu=push?push->menu():nullptr;
        result["push_button_interface"]=push!=nullptr;
        result["menu_attached"]=menu!=nullptr;
        if (menu) {
            result["menu"]=widgetState(menu,false);
            result["menu_actions"]=menu->actions().size();
            result["menu_signals_blocked"]=menu->signalsBlocked();
            result["menu_embedded"]=menu->graphicsProxyWidget()!=nullptr;
            result["menu_window_flags"]=double(quint32(menu->windowFlags()));
            result["menu_parent_is_button"]=menu->parentWidget()==button;
        }
        const auto global=button->mapToGlobal(button->rect().center());
        result["center"]=QJsonArray{global.x(),global.y()};
        result["center_hit"]=widgetState(QApplication::widgetAt(global),false);
        result["active_popup"]=widgetState(QApplication::activePopupWidget(),false);
        return result;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        auto button=qobject_cast<QAbstractButton *>(object);
        if (!button || !buttons.contains(button)) return false;
        const auto type=event->type();
        if (type==QEvent::MouseButtonPress || type==QEvent::MouseButtonRelease) {
            auto mouse=static_cast<QMouseEvent *>(event);
            record(button,type==QEvent::MouseButtonPress?"mouse-press":"mouse-release",
                   {{"mouse_button",int(mouse->button())},
                    {"local",QJsonArray{mouse->pos().x(),mouse->pos().y()}},
                    {"global",QJsonArray{mouse->globalPos().x(),mouse->globalPos().y()}}});
            QPointer<QAbstractButton> alive=button;
            QTimer::singleShot(0,this,[this,alive]{if(alive)record(alive,"after-input");});
        }
        return false;
    }
private:
    QSet<QAbstractButton *> buttons;
    void record(QAbstractButton *button,const QString &phase,QJsonObject extra={}) {
        auto state=capture(button);state["phase"]=phase;
        for(auto it=extra.begin();it!=extra.end();++it)state[it.key()]=it.value();
        logEvent("project-selector.native-state",state);
    }
};
}
