// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QTimer>
#include <QHash>
#include <QSet>
#include <functional>
namespace Unbound {
// Keep an OpenGate action out of the native palette's broad triggered/hovered
// handlers. Native action connections stay intact. Leases end next turn or before
// any subsequent input, and retain original signal-block state across overlaps.
class IsolatedNativeAction : public QObject {
public:
    static QAction *create(const QString &label,QObject *owner,std::function<void()> invoke) {
        auto action=new QAction(label,owner);
        // Must connect before QMenu::addAction installs its signal propagation.
        connect(action,&QAction::triggered,owner,[action,invoke=std::move(invoke)]{shared()->begin(action);invoke();});
        connect(action,&QAction::hovered,owner,[action]{shared()->begin(action);});
        return action;
    }
    ~IsolatedNativeAction() override {restore();if(qApp)qApp->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        // QMenu::hideEvent emits aboutToHide. Native menu buttons depend on
        // that lifecycle notification even when tracking ends without input.
        if(event->type()==QEvent::Hide && leases.contains(object))restore();
        switch(event->type()) {
        case QEvent::MouseButtonPress:case QEvent::MouseButtonRelease:case QEvent::KeyPress:
        case QEvent::Shortcut:restore();break;
        default:break;
        }
        return false;
    }
private:
    struct Lease {QPointer<QObject> object;bool blocked=false;int depth=0;};
    QHash<QObject *,Lease> leases;quint64 generation=0;
    explicit IsolatedNativeAction(QObject *owner):QObject(owner){qApp->installEventFilter(this);}
    static IsolatedNativeAction *shared(){static QPointer<IsolatedNativeAction> value;
        if(!value)value=new IsolatedNativeAction(qApp);return value;}
    void begin(QAction *action) {
        QVector<QPointer<QObject>> token;
        auto pending=action?action->associatedWidgets():QList<QWidget *>();
        QSet<QWidget *> seen;
        while(!pending.isEmpty()) {
            auto widget=pending.takeFirst();
            if(!widget||seen.contains(widget))continue;
            seen.insert(widget);
            auto menu=qobject_cast<QMenu *>(widget);
            if(!menu&&!qobject_cast<QMenuBar *>(widget))continue;
            auto &lease=leases[widget];if(!lease.object)lease={widget,widget->blockSignals(true),0};
            ++lease.depth;token.append(widget);
            // QMenu propagates through the widgets hosting its menuAction,
            // which need not be its QObject parent. Other native menus are
            // never part of this action's signal propagation chain.
            if(menu)pending.append(menu->menuAction()->associatedWidgets());
        }
        const auto serial=generation;
        QTimer::singleShot(0,this,[this,token,serial]{
            if(serial!=generation)return;
            for(auto object:token)if(object && leases.contains(object)){
                auto &lease=leases[object];if(--lease.depth==0){object->blockSignals(lease.blocked);leases.remove(object);}}
        });
    }
    void restore(){++generation;for(const auto &lease:leases)if(lease.object)lease.object->blockSignals(lease.blocked);leases.clear();}
};
}
