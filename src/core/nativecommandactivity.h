// SPDX-License-Identifier: MIT
#pragma once
#include <QAction>
#include <QActionEvent>
#include <QApplication>
#include <QPointer>
#include <QSet>
#include <QShortcut>
#include <QStringList>
#include <QTimer>
#include <functional>

namespace Unbound {
// One observation path for native actions/shortcuts and completed OpenGate
// dispatch. It never chooses keys, executes commands, or owns native edit state.
class NativeCommandActivity : public QObject {
public:
    static NativeCommandActivity *shared() {
        auto &instance=storage();
        if(!instance && qApp && !QCoreApplication::closingDown())instance=new NativeCommandActivity;
        return instance;
    }
    static NativeCommandActivity *existing(){return storage();}
    using Callback=std::function<void(const QString &)>;
    using ActionCallback=std::function<void(QAction *)>;
    quint64 subscribeActions(QObject *owner,ActionCallback callback) {
        const auto id=++nextId;actionListeners.append({id,owner,std::move(callback)});return id;
    }
    quint64 subscribe(QObject *owner,Callback callback) {
        const auto id=++nextId;listeners.append({id,owner,std::move(callback)});return id;
    }
    void unsubscribe(quint64 id) {
        for(int i=listeners.size()-1;i>=0;--i)if(listeners[i].id==id || !listeners[i].owner)listeners.removeAt(i);
        for(int i=actionListeners.size()-1;i>=0;--i)if(actionListeners[i].id==id || !actionListeners[i].owner)actionListeners.removeAt(i);
    }
    void completed(const QString &source) {
        if(pendingSources.size()<16 && !pendingSources.contains(source))pendingSources.append(source);
        if(pending)return;pending=true;
        QTimer::singleShot(0,this,[this] {
            pending=false;const auto source=pendingSources.join(QStringLiteral(" | "));pendingSources.clear();const auto current=listeners;
            for(const auto &entry:current)if(entry.owner && entry.callback)entry.callback(source);
            unsubscribe(0);
        });
    }
    void observe(QObject *object) {
        if(!object || tracked.contains(object))return;
        auto action=qobject_cast<QAction *>(object);auto shortcut=qobject_cast<QShortcut *>(object);
        if(!action && !shortcut)return;
        tracked.insert(object);connect(object,&QObject::destroyed,this,[this,object]{tracked.remove(object);});
        if(action)connect(action,&QAction::triggered,this,[this,alive=QPointer<QAction>(action)] {
            const auto current=actionListeners;
            for(const auto &entry:current)if(alive && entry.owner && entry.callback)entry.callback(alive);
            completed(alive?alive->objectName():QStringLiteral("native-action"));
        });
        if(shortcut)connect(shortcut,&QShortcut::activated,this,[this,shortcut]{
            completed(shortcut->objectName().isEmpty()?QStringLiteral("shortcut:")+shortcut->key().toString(QKeySequence::PortableText):shortcut->objectName());
        });
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::ActionAdded)observe(static_cast<QActionEvent *>(event)->action());
        if((event->type()==QEvent::ChildAdded || event->type()==QEvent::Show) && qobject_cast<QWidget *>(object))scanLater(object);
        return false;
    }
private:
    static QPointer<NativeCommandActivity> &storage(){static QPointer<NativeCommandActivity> instance;return instance;}
    NativeCommandActivity():QObject(qApp) {
        qApp->installEventFilter(this);
        QTimer::singleShot(0,this,[this]{for(auto window:QApplication::topLevelWidgets())scan(window);});
    }
    struct Listener {quint64 id;QPointer<QObject> owner;Callback callback;};
    struct ActionListener {quint64 id;QPointer<QObject> owner;ActionCallback callback;};
    void scan(QObject *root) {
        observe(root);
        for(auto action:root->findChildren<QAction *>())observe(action);
        for(auto shortcut:root->findChildren<QShortcut *>())observe(shortcut);
    }
    void scanLater(QObject *root) {
        for(auto entry:roots)for(auto parent=root;parent;parent=parent->parent())if(entry==parent)return;
        for(int i=roots.size()-1;i>=0;--i) {
            if(!roots[i]){roots.removeAt(i);continue;}
            for(auto parent=roots[i]->parent();parent;parent=parent->parent())if(parent==root){roots.removeAt(i);break;}
        }
        roots.append(root);if(scanPending)return;scanPending=true;
        QTimer::singleShot(0,this,[this]{scanPending=false;const auto next=roots;roots.clear();for(auto root:next)if(root)scan(root);});
    }
    QSet<QObject *> tracked;QList<Listener> listeners;QList<ActionListener> actionListeners;QList<QPointer<QObject>> roots;
    quint64 nextId=0;bool pending=false,scanPending=false;QStringList pendingSources;
};
}
