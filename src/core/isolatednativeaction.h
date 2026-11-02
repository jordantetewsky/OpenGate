// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QTimer>
#include <QHash>
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
        connect(action,&QAction::triggered,owner,[invoke=std::move(invoke)]{shared()->begin();invoke();});
        connect(action,&QAction::hovered,owner,[]{shared()->begin();});
        return action;
    }
    ~IsolatedNativeAction() override {restore();if(qApp)qApp->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *,QEvent *event) override {
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
    void begin() {
        QVector<QPointer<QObject>> token;
        for(auto widget:QApplication::allWidgets())if(qobject_cast<QMenu *>(widget)||qobject_cast<QMenuBar *>(widget)) {
            auto &lease=leases[widget];if(!lease.object)lease={widget,widget->blockSignals(true),0};
            ++lease.depth;token.append(widget);
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
