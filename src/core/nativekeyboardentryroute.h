// SPDX-License-Identifier: MIT
#pragma once
#include "keyboardlauncher.h"
#include "isolatednativeaction.h"
#include "menutrackingcommand.h"
#include "panelcommandcontext.h"
#include "nativeshortcutcontext.h"
#include "../platform/nativekeyboardmenuroute.h"
#include <QActionEvent>
#include <QShortcut>
#include <QShortcutEvent>
#include <QScopedValueRollback>

namespace Unbound {
// Lease the native menu row only while tracking; adapt its already-selected
// shortcut without changing Resolve's binding database or native connections.
class NativeKeyboardEntryRoute : public QObject {
public:
    explicit NativeKeyboardEntryRoute(QObject *owner=nullptr):QObject(owner) {
        qApp->installEventFilter(this);
        nativeMenu=createNativeKeyboardMenuRoute(this,[this]{return invokePanel();});
        refresh();
    }
    ~NativeKeyboardEntryRoute() override {
        if(qApp)qApp->removeEventFilter(this);
        restore();
    }
    std::function<bool()> enabled;
    std::function<bool()> openPanel;
    std::function<QStringList(const QKeySequence &)> shortcutOwners;
    void abandonForQuit() {
        abandoned=true;enabled={};openPanel={};shortcutOwners={};
        if(nativeMenu)nativeMenu->abandonForQuit();
        if(qApp)qApp->removeEventFilter(this);
    }
    void refresh() {
        if(abandoned || changing)return;
        for(auto widget:QApplication::allWidgets())if(auto menu=qobject_cast<QMenu *>(widget)) {
            if(menu->property("unboundOwnMenu").toBool() || menus.contains(menu))continue;
            menus.insert(menu);
            connect(menu,&QObject::destroyed,this,[this,menu]{menus.remove(menu);});
            connect(menu,&QMenu::aboutToShow,this,[this,menu]{prepare(menu);});
            connect(menu,&QMenu::aboutToHide,this,[this,menu]{
                QTimer::singleShot(0,this,[this,menu]{if(leasedMenu==menu)restore();});
            });
        }
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(abandoned)return false;
#ifdef Q_OS_MAC
        if(event->type()==QEvent::ShortcutOverride || event->type()==QEvent::KeyPress) {
            auto key=static_cast<QKeyEvent *>(event);
            const QKeySequence pressed(key->key()|int(key->modifiers() & ~Qt::KeypadModifier));
            if(pressed==QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_K) && eligible() &&
               PanelCommandContext::blocked().isEmpty() && aliasAvailable(pressed)) {
                event->accept();
                if(event->type()==QEvent::KeyPress && !key->isAutoRepeat())
                    QTimer::singleShot(0,this,[this,pressed]{
                        if(!eligible() || !aliasAvailable(pressed) || !PanelCommandContext::blocked().isEmpty())return;
                        if(!invokePanel())if(auto action=nativeAction())action->trigger();
                    });
                return true;
            }
        }
#endif
        if(event->type()==QEvent::ActionAdded || event->type()==QEvent::Show) {
            if(!changing && !refreshQueued) {
                refreshQueued=true;
                QTimer::singleShot(0,this,[this]{refreshQueued=false;refresh();});
            }
        }
        if(event->type()!=QEvent::Shortcut || dispatching || !eligible())return false;
        auto shortcutEvent=static_cast<QShortcutEvent *>(event);
        if(shortcutEvent->isAmbiguous())return false;
        auto native=nativeAction();
        if(!native || !native->isEnabled())return false;
        bool match=object==native && native->shortcuts().contains(shortcutEvent->key());
        if(auto shortcut=qobject_cast<QShortcut *>(object)) {
            if(shortcut->isEnabled() && shortcut->id()==shortcutEvent->shortcutId() &&
               shortcut->key()==shortcutEvent->key() && shortcutOwners && !native->objectName().isEmpty()) {
                const auto owners=shortcutOwners(shortcutEvent->key());
                match=owners.size()==1 && owners.front()==native->objectName();
            }
        }
        if(!match)return false;
        const QScopedValueRollback<bool> routing(dispatching,true);
        restore();
        if(!openPanel())return false; // Qt delivers its unchanged native event.
        event->accept();return true;
    }
private:
    NativeKeyboardMenuRoute *nativeMenu=nullptr;
    QSet<QMenu *> menus;
    QPointer<QMenu> leasedMenu;
    QPointer<QAction> original,mirror;
    bool changing=false,dispatching=false,refreshQueued=false,abandoned=false;
    bool eligible() const {return !abandoned && openPanel && (!enabled || enabled());}
    bool invokePanel() {
        if(dispatching || !eligible())return false;
        const QScopedValueRollback<bool> routing(dispatching,true);
        restore();return openPanel();
    }
    bool aliasAvailable(const QKeySequence &key) const {
        if(!shortcutOwners)return false;
        const auto native=nativeAction();
        for(const auto &owner:shortcutOwners(key)) {
            if(native && owner==native->objectName())continue;
            const auto scope=NativeShortcutContext::describe(owner);
            if(!scope.known || scope.page.isEmpty() || scope.page=="edit")return false;
        }
        return true;
    }
    QAction *nativeAction() const {
        return original ? original.data() : nativeKeyboardCustomizationAction();
    }
    void prepare(QMenu *menu) {
        if(!eligible() || changing)return;
        auto action=nativeAction();
        if(!action || !action->isEnabled() || !menu->actions().contains(action))return;
        restore();
        const QScopedValueRollback<bool> mutation(changing,true);
        original=action;leasedMenu=menu;
        mirror=IsolatedNativeAction::create(action->text(),this,[this]{
            const QPointer<QAction> fallback=original;
            MenuTrackingCommand::queue(this,[this,fallback]{
                if(abandoned)return;
                restore(); // Native editor catalogue must see its original row.
                if(dispatching)return;
                const QScopedValueRollback<bool> routing(dispatching,true);
                if(eligible() && openPanel())return;
                if(fallback && fallback->isEnabled()) {
                    WorkflowCommandMenu::prepareKeyboardEditor();
                    fallback->trigger(); // Original has no replacement signal connection.
                }
            });
        });
        mirror->setObjectName("openGateNativeKeyboardEntry");
        mirror->setProperty("openGateNativeKeyboardEntry",true);
        mirror->setIcon(action->icon());mirror->setEnabled(action->isEnabled());
        mirror->setMenuRole(action->menuRole());
        // Preserve the displayed assignment without registering a second key.
        if(!action->shortcut().isEmpty() && !mirror->text().contains('\t'))
            mirror->setText(action->text()+"\t"+action->shortcut().toString(QKeySequence::NativeText));
        menu->insertAction(action,mirror);menu->removeAction(action);
    }
    void restore() {
        if(abandoned)return;
        const QScopedValueRollback<bool> mutation(changing,true);
        if(leasedMenu && original && mirror && leasedMenu->actions().contains(mirror))
            leasedMenu->insertAction(mirror,original);
        if(mirror)delete mirror;
        mirror=nullptr;original=nullptr;leasedMenu=nullptr;
    }
};
} // namespace Unbound
