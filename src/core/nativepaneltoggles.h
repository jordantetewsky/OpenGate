// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QHash>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QSignalBlocker>
#include <QTimer>
#include <functional>
#include <memory>
#include <vector>
#include "nativepanelactivation.h"
#include "diagnostics.h"
namespace Unbound {
// Native controls are callers while OpenGate owns their surfaces. Native state
// notifications only refresh presentation; they must never become new intent.
class NativePanelToggles : public QObject {
public:
    explicit NativePanelToggles(QObject *owner) : QObject(owner) {}
    void add(QString toggle,std::function<bool()> managed,std::function<bool()> shown,
             std::function<void()> request)
    { entries.append({std::move(toggle),std::move(managed),std::move(shown),std::move(request)}); }
    void addMatching(QString id,std::function<bool(QAbstractButton *)> matches,
                     std::function<bool()> managed,std::function<bool()> shown,std::function<void()> request) {
        entries.append({std::move(id),std::move(managed),std::move(shown),std::move(request),std::move(matches)});
    }
    std::function<bool()> blocked;
    void sync(QWidget *root,bool nativeSnapshot=false) { nativeRoot=root;sync(nativeSnapshot); }
    void wire(QWidget *root) {
        if (!root) return;nativeRoot=root;
        for (auto button:root->window()->findChildren<QAbstractButton *>()) {
            for (int i=0;i<entries.size();++i) {
                const auto &entry=entries[i];
                const bool matches=entry.matches ? entry.matches(button) : !entry.toggle.isEmpty() && button->objectName().startsWith(QStringLiteral("btnEditToggle")+entry.toggle+"In");
                if(!matches || observed.contains(button))continue;
                observed.insert(button,i);button->installEventFilter(this);
                connect(button,&QObject::destroyed,this,[this,button]{observed.remove(button);});
                connect(button,&QAbstractButton::clicked,this,[this,button,i]{activate(button,i);});
                connect(button,&QAbstractButton::toggled,this,[this](bool){schedule();});
            }
        }
        sync();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        const auto type=event->type();
        if(type!=QEvent::MouseButtonPress && type!=QEvent::MouseButtonRelease && type!=QEvent::KeyPress && type!=QEvent::KeyRelease && type!=QEvent::Shortcut)return false;
        auto button=qobject_cast<QAbstractButton *>(object);
        if(!button || !observed.contains(button))return false;
        const int i=observed.value(button);
        if(!entries[i].managed() || nativePanelActivationDepth || !currentNativePanelControl(button,nativeRoot ? nativeRoot->window() : nullptr))return false;
        // Keep native press/release handlers from operating on leased slots.
        // PanelTitleGesture still observes the press and owns threshold drags.
        if(event->type()==QEvent::MouseButtonPress) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(mouse->button()==Qt::LeftButton){button->setDown(true);return true;}
        } else if(event->type()==QEvent::MouseButtonRelease) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(mouse->button()==Qt::LeftButton){const bool invoke=button->isDown() && button->rect().contains(mouse->pos());button->setDown(false);if(invoke)activate(button,i);return true;}
        } else if(event->type()==QEvent::KeyPress || event->type()==QEvent::KeyRelease) {
            auto key=static_cast<QKeyEvent *>(event);
            if(key->key()==Qt::Key_Space){if(event->type()==QEvent::KeyRelease && !key->isAutoRepeat())activate(button,i);return true;}
        } else if(event->type()==QEvent::Shortcut){activate(button,i);return true;}
        return false;
    }
private:
    struct Entry { QString toggle; std::function<bool()> managed,shown;std::function<void()> request;std::function<bool(QAbstractButton *)> matches={}; };
    bool unavailable() const {return nativePanelActivationDepth || (blocked && blocked());}
    void activate(QAbstractButton *button,int i) {
        if(unavailable() || !entries[i].managed() || !nativeRoot || !currentNativePanelControl(button,nativeRoot->window()))return;
        logEvent("panel.caller.native-activation",{{"button",button->objectName()},{"toggle",entries[i].toggle}});
        entries[i].request();schedule();
    }
    void schedule() {
        if(refreshQueued)return;refreshQueued=true;
        QTimer::singleShot(0,this,[this]{refreshQueued=false;sync();});
    }
    void sync(bool nativeSnapshot=false) {
        if(!nativeRoot || unavailable())return;
        // Block every managed peer before changing any value. Native radio-like
        // groups may uncheck a different button as a side effect of setChecked.
        std::vector<std::unique_ptr<QSignalBlocker>> blockers;
        for(auto it=observed.begin();it!=observed.end();++it)if(nativeSnapshot || entries[it.value()].managed())blockers.emplace_back(new QSignalBlocker(it.key()));
        QSet<QButtonGroup *> exclusiveGroups;QList<QAbstractButton *> exclusiveButtons;
        for(auto it=observed.begin();it!=observed.end();++it)if(nativeSnapshot || entries[it.value()].managed()) {
            if(auto group=it.key()->group())if(group->exclusive()){exclusiveGroups.insert(group);group->setExclusive(false);}
            if(it.key()->autoExclusive()){exclusiveButtons.append(it.key());it.key()->setAutoExclusive(false);}
        }
        for(auto it=observed.begin();it!=observed.end();++it){const auto &entry=entries[it.value()];if(nativeSnapshot || entry.managed())it.key()->setChecked(entry.shown());}
        for(auto button:exclusiveButtons)button->setAutoExclusive(true);
        for(auto group:exclusiveGroups)group->setExclusive(true);
    }
    QPointer<QWidget> nativeRoot;
    QHash<QAbstractButton *,int> observed;
    QList<Entry> entries;
    bool refreshQueued=false;
};
}
