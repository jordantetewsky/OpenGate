// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QActionEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QShortcut>
#include <QShortcutEvent>
#include <QTimer>
#include <functional>
#include "nativeeditmenutarget.h"
#include "diagnostics.h"

namespace Unbound {
// Translate explicit native navigation into existing panel-door requests.
// Resolve owns command execution; the workspace owns placement and visibility.
class NativePanelNavigation : public QObject {
public:
    struct Context {
        QString blocked;
        QPointer<QWidget> page,pool,surface;
        QPointer<QObject> destination;
        unsigned version=0;
        bool shown=false;
    };
    explicit NativePanelNavigation(QObject *owner):QObject(owner) {}
    std::function<Context(const QString &)> context;
    std::function<void(QObject *)> reveal;
    std::function<QStringList(const QKeySequence &)> shortcutOwners;
    void scanActions(QWidget *root) {
        if(!root)return;
        for(auto action:root->actions())wire(action);
        for(auto action:root->findChildren<QAction *>())wire(action);
    }
    void observe(QObject *object,QEvent *event) {
        const auto type=event->type();
        if(type==QEvent::Shortcut) {
            auto shortcut=qobject_cast<QShortcut *>(object);
            auto dispatch=static_cast<QShortcutEvent *>(event);
            if(shortcut && !dispatch->isAmbiguous() && shortcut->isEnabled() &&
               shortcut->id()==dispatch->shortcutId() && shortcut->key()==dispatch->key())wireShortcut(shortcut);
        }
        if(type==QEvent::ActionAdded)wire(static_cast<QActionEvent *>(event)->action());
        if(type==QEvent::Show) {
            auto widget=qobject_cast<QWidget *>(object);
            if(widget && (widget->objectName()=="UiMainWindow" || qobject_cast<QMenu *>(widget)))scanActions(widget);
        }
        // A custom-view activation is valid only within its explicit pointer
        // gesture. Native button signals do not generate these pointer events.
        if(type==QEvent::MouseButtonRelease && !gestureConnections.isEmpty()) {
            // Resolve's observed load activation occurs in release handling,
            // after the double-click's first queued event turn.
            const auto generation=gestureGeneration;
            QTimer::singleShot(0,this,[this,generation]{if(generation==gestureGeneration)cancelGesture();});
        }
        if(type==QEvent::MouseButtonPress ||
           type==QEvent::KeyPress || type==QEvent::ApplicationDeactivate)cancelGesture();
        if(type!=QEvent::MouseButtonDblClick)return;
        cancelGesture();
        auto mouse=static_cast<QMouseEvent *>(event);
        auto viewport=qobject_cast<QWidget *>(object);
        auto parent=viewport ? viewport->parentWidget() : nullptr;
        if(!parent || parent->objectName()!="mediaPoolClipView" || mouse->button()!=Qt::LeftButton)return;
        const auto before=context(sourceId());
        if(!eligible(before,"media-pool-double-click"))return;
        if(!before.pool || !before.pool->isAncestorOf(parent)){skip("pool-outside-managed-owner");return;}
        if(auto view=qobject_cast<QAbstractItemView *>(parent)) {
            if(view->viewport()!=viewport || !view->indexAt(mouse->pos()).isValid()){skip("no-item-at-pointer");return;}
            request(sourceId(),before,"media-pool-double-click");return;
        }
        auto scroll=qobject_cast<QAbstractScrollArea *>(parent);
        if(!scroll || scroll->viewport()!=viewport){skip("unsupported-clip-viewport");return;}
        // Resolve's custom clip view is not QAbstractItemView. Its native load
        // path activates Source Clip even for the same clip and a hidden viewer.
        // Observe that activation; never click the button or infer an item index.
        auto player=before.surface ? before.surface->findChild<QWidget *>("widgetEditSourcePlayer") : nullptr;
        if(before.surface && before.surface->objectName()=="widgetEditSourcePlayer")player=before.surface;
        if(!player){skip("source-player-unavailable");return;}
        for(auto button:player->findChildren<QAbstractButton *>("btnSourceClipMode")) {
            gestureConnections.append(connect(button,&QAbstractButton::clicked,this,[this,before] {
                if(gestureAccepted)return;
                gestureAccepted=true;
                request(sourceId(),before,"media-pool-native-source-activation");
            }));
        }
        if(gestureConnections.isEmpty())skip("source-activation-control-unavailable");
        else logEvent("source.navigation.armed",{{"view_class",parent->metaObject()->className()}});
    }
private:
    static QString sourceId(){return QStringLiteral("widgetEditSourcePlayer");}
    static void skip(const QString &reason,const QString &origin={}) {
        logEvent("navigation.reveal.skipped",{{"reason",reason},{"origin",origin}});
    }
    bool eligible(const Context &state,const QString &origin) const {
        if(!state.blocked.isEmpty()){skip(state.blocked,origin);return false;}
        if(!state.destination){skip("destination-not-leased",origin);return false;}
        if(state.shown)return false;
        return true;
    }
    void cancelGesture() {
        ++gestureGeneration;
        for(const auto &connection:gestureConnections)disconnect(connection);
        gestureConnections.clear();gestureAccepted=false;
    }
    void request(const QString &id,const Context &before,const QString &origin) {
        logEvent("navigation.reveal.queued",{{"panel",id},{"origin",origin}});
        QTimer::singleShot(0,this,[this,id,before,origin] {
            const auto now=context(id);
            if(!eligible(now,origin))return;
            if(!before.page || before.page!=now.page || before.version!=now.version ||
               !before.destination || before.destination!=now.destination){skip("ownership-or-transaction-changed",origin);return;}
            reveal(now.destination);
            logEvent("navigation.reveal.complete",{{"panel",id},{"origin",origin}});
        });
    }
    static QString destination(QAction *action) {
        if(!action || action->property("unboundOwnMenu").toBool())return {};
        for(auto parent=action->parent();parent;parent=parent->parent())
            if(parent->property("unboundOwnMenu").toBool())return {};
        if(nativeEditActionLabel(action,"Find in Media Pool"))return QStringLiteral("frameEditPageMediaPool");
        if(nativeEditActionLabel(action,"Match Frame"))return sourceId();
        return {};
    }
    void wire(QAction *action) {
        if(!action || actions.contains(action))return;
        const auto id=destination(action);if(id.isEmpty())return;
        actions.insert(action);
        connect(action,&QObject::destroyed,this,[this,action]{actions.remove(action);});
        connect(action,&QAction::triggered,this,[this,action] {
            const auto id=destination(action);if(id.isEmpty())return;
            const auto before=context(id);
            logEvent("navigation.native-action",{{"action",action->objectName()},{"label",action->text()},{"panel",id}});
            if(eligible(before,action->objectName()))request(id,before,action->objectName());
        });
    }
    bool isFindShortcut(const QKeySequence &key) const {
        if(!shortcutOwners)return false;
        bool find=false;
        for(const auto &owner:shortcutOwners(key)) {
            if(owner=="clipFindClipInMediaPool" || owner=="MediaPool.clipFindClipInMediaPool")find=true;
            // These preset entries belong to other pages; the context below
            // must still be the visible Edit page. Unknown conflicts fail closed.
            else if(!owner.startsWith("FusionWidget.") && !owner.startsWith("FairlightTimeline."))return false;
        }
        return find;
    }
    void wireShortcut(QShortcut *shortcut) {
        if(shortcuts.contains(shortcut) || !isFindShortcut(shortcut->key()))return;
        shortcuts.insert(shortcut);
        connect(shortcut,&QObject::destroyed,this,[this,shortcut]{shortcuts.remove(shortcut);});
        connect(shortcut,&QShortcut::activated,this,[this,shortcut] {
            // Observe actual native activation; never intercept/replay the key.
            // Re-read current assignment so a preset/key change cannot retain
            // a stale Find association on an existing native QShortcut.
            if(!isFindShortcut(shortcut->key()))return;
            const QString id="frameEditPageMediaPool";
            const auto before=context(id);
            if(!before.page || before.page->objectName()!="WStackPage_Conform")return;
            logEvent("navigation.native-shortcut",{{"command","clipFindClipInMediaPool"},
                {"key",shortcut->key().toString(QKeySequence::PortableText)}});
            if(eligible(before,"clipFindClipInMediaPool"))request(id,before,"clipFindClipInMediaPool");
        });
    }
    QSet<QAction *> actions;
    QSet<QShortcut *> shortcuts;
    QList<QMetaObject::Connection> gestureConnections;
    bool gestureAccepted=false;
    unsigned gestureGeneration=0;
};
}
