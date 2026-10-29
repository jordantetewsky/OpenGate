// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QAction>
#include <QBoxLayout>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QScopedValueRollback>
#include <memory>
#include <QTimer>
#include "diagnostics.h"
#include "pagecontext.h"
#include "surfacepagenavigation.h"
#include "nativefootermirror.h"
#include "panelaccessorylease.h"
namespace Unbound {
// Global visibility is Resolve's command. Surface placement is OpenGate state.
// The original complete footer occupies the primary surface through a reversible
// bottom-band lease; secondary surfaces invoke the original native buttons.
class FooterPanel : public QObject {
public:
    FooterPanel(QObject *owner,std::function<QWidget *()> mainWindow,QJsonObject saved)
        :QObject(owner),nativeLease(this),root(std::move(mainWindow)),state(std::move(saved)) {
        timer.setSingleShot(true);timer.setInterval(40);connect(&timer,&QTimer::timeout,this,[this]{refresh();});
        pendingVisibility=state.contains("hidden");qApp->installEventFilter(this);schedule();
    }
    ~FooterPanel() override {shutdown();}
    std::function<void()> changed,geometryChanged;
    QRect externalBounds(QWidget *host) const {
        if(!shown || !nativeFooter || !nativeFooter->isVisible() || nativeFooter->window()==host)return {};
        const auto own=views.value(host);if(own && own->isVisible())return {};
        if(nativeLease.active())return nativeLease.visibleBounds();
        return QRect(nativeFooter->mapToGlobal(QPoint()),nativeFooter->size());
    }
    std::function<QWidget *()> primarySurface;
    std::function<void(bool,bool)> placementChanged;
    std::function<QList<QWidget *>()> surfaceHosts;
    void shutdown() {
        stopped=true;timer.stop();qApp->removeEventFilter(this);nativeLease.release();
        for(auto view:views)if(view)delete view;views.clear();
        if(nativeFooter){QScopedValueRollback<bool> guard(applyingPlacement,true);nativeFooter->setVisible(nativeVisibility?nativeVisibility->isChecked():shown);}
    }
    bool releaseNative(){return nativeLease.release();}
    void refreshSurfaces(){refresh();}
    void restore(){refresh();}
    void wireMenu(){searchedCaller=false;refresh();}
    QString pinPrimarySurface(bool enabled) {
        auto host=primarySurface?primarySurface():nullptr;
        if(!host)return QStringLiteral("No workspace surface is available yet.");
        setPinned(host,enabled);return {};
    }
    QString showPanel(){return requestVisibility(true);}
    QString hidePanel(){return requestVisibility(false);}
    QString toggleVisibility(){refresh();return requestVisibility(!shown);}
    QJsonObject snapshot() const {
        auto value=state;value.remove("surfaces");value["model"]="reserved-native-bands-v2";value["floating"]=false;value["attachment"]="surfaces";value["hidden"]=!shown;return value;
    }
    void setState(const QJsonObject &value) {
        state=value;pendingVisibility=value.contains("hidden");refresh();
    }
    bool pinned(QWidget *host) const {return host && !host->property("unboundToolbarStrip").toBool();}
    // Legacy API retained for older callers. Footer is now a reserved band on
    // every workspace surface; Show/Hide is the only placement choice.
    void setPinned(QWidget *,bool enabled) {requestVisibility(enabled);}
    void populatePlacementMenu(QMenu *,QWidget *) {}
    void populateSurfaceMenu(QMenu *menu,QWidget *) {
        auto visibility=menu->addAction("Footer");visibility->setProperty("openGateNavigationProxy",true);
        visibility->setCheckable(true);visibility->setChecked(shown);
        connect(visibility,&QAction::triggered,this,[this](bool value){defer([this,value]{requestVisibility(value);});});
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        const auto type=event->type();
        if(type==QEvent::Show || type==QEvent::Hide || type==QEvent::Resize || type==QEvent::LayoutRequest) {
            auto widget=qobject_cast<QWidget *>(object);
            if(widget && widget==nativeFooter && !nativeLease.active() && !applyingPlacement && !nativeVisibility && initialized && (type==QEvent::Show || type==QEvent::Hide)) {
                shown=!nativeFooter->isHidden();publish();
            }
            if(widget && widget->objectName()=="workspaceShowPanel")nativeCaller=qobject_cast<QMenu *>(widget);
            if(widget && (widget==nativeFooter || widget->objectName()=="m_PageSwitcherContainer" || widget->objectName()=="frameControlPanelMiddle" || widget->isWindow() || widget->objectName().startsWith("WStackPage_") || widget->objectName()=="workspaceShowPanel"))schedule();
        }
        return false;
    }
private:
    void schedule(){if(!timer.isActive())timer.start();}
    void publish(){if(state.contains("hidden") && state["hidden"].toBool()==!shown)return;state["hidden"]=!shown;if(changed)changed();}
    void defer(std::function<void()> operation) {
        auto wait=new QTimer(this);wait->setSingleShot(true);auto attempts=std::make_shared<int>(0);
        connect(wait,&QTimer::timeout,this,[wait,attempts,operation] {
            if(QApplication::activePopupWidget() && ++*attempts<600){wait->start(25);return;}
            if(!QApplication::activePopupWidget())operation();wait->deleteLater();
        });wait->start(0);
    }
    QString requestVisibility(bool value) {
        logEvent("footer.visibility.request",{{"visible",value}});
        discover();
        if(nativeVisibility) {
            if(nativeVisibility->isChecked()!=value) {
                if(!nativeVisibility->isEnabled()){logEvent("footer.visibility.failed",{{"reason","native-command-disabled"},{"visible",value}});return QStringLiteral("Resolve's Show Page Navigation command is disabled.");}
                if(QApplication::activePopupWidget()){defer([this,value]{requestVisibility(value);});return {};}
                nativeVisibility->trigger();
            }
            shown=nativeVisibility->isChecked();
        } else if(nativeFooter) {
            // Fallback for early construction: this is still the real native
            // footer, in its original slot. A later native action takes authority.
            nativeFooter->setVisible(value);shown=value;
        } else {logEvent("footer.visibility.failed",{{"reason","native-footer-unavailable"},{"visible",value}});return QStringLiteral("Native Page Navigation is not available yet.");}
        pendingVisibility=!nativeVisibility;publish();refresh();
        logEvent("footer.visibility.complete",{{"requested",value},{"visible",shown}});return {};
    }
    void discover() {
        auto main=root?root():nullptr;if(!main)return;
        if(!nativeFooter)nativeFooter=main->findChild<QWidget *>("m_PageSwitcherContainer");
        QSet<QAction *> matches;
        if(!nativeVisibility && (!actionSearch.isValid() || actionSearch.elapsed()>=250)) {
        actionSearch.restart();
        for(auto action:main->findChildren<QAction *>()) {
            QString text=action->text();text.remove('&');
            if(action->isCheckable() && text.section('\t',0,0).trimmed()=="Show Page Navigation" && !action->property("openGateNavigationProxy").toBool())matches.insert(action);
        }
        }
        if(matches.size()==1 && nativeVisibility!=*matches.begin()) {
            nativeVisibility=*matches.begin();
            connect(nativeVisibility,&QAction::toggled,this,[this](bool value){shown=value;pendingVisibility=false;publish();schedule();});
        }
        if(!searchedCaller) {
            searchedCaller=true;
            nativeCaller=main->findChild<QMenu *>("workspaceShowPanel");
            if(!nativeCaller)for(auto widget:QApplication::allWidgets())if(auto menu=qobject_cast<QMenu *>(widget))
                if(menu->objectName()=="workspaceShowPanel"){nativeCaller=menu;break;}
        }
    }
    void refresh() {
        if(stopped || refreshing)return;QScopedValueRollback<bool> guard(refreshing,true);discover();
        if(pendingVisibility && nativeFooter)requestVisibility(!state["hidden"].toBool());
        else if(nativeVisibility)shown=nativeVisibility->isChecked();
        else if(nativeFooter && !initialized)shown=!nativeFooter->isHidden();
        initialized=true;
        QPointer<QWidget> destination=primarySurface?primarySurface():nullptr;
        const auto nativeMain=root?root():nullptr;
        if(nativeFooter) {
            QScopedValueRollback<bool> placement(applyingPlacement,true);
            const bool custom=destination && destination!=nativeMain && pinned(destination);
            if(nativeLease.active() && (!custom || nativeLease.ownerSurface()!=destination)) {
                if(!nativeLease.release()){logEvent("footer.native.release.failed",{{"reason","native-slot-ownership-changed"}});return;}
            }
            if(custom && shown && !nativeLease.active()) {
                nativeFooter->show();
                const auto error=nativeLease.attach(nativeFooter,nativeMain,destination,Qt::BottomEdge,false);
                if(!error.isEmpty())logEvent("footer.native.mount.failed",{{"error",error}});
            }
            if(nativeLease.active()){nativeLease.setHidden(!shown);nativeFooter->setVisible(shown);}
            else nativeFooter->setVisible(shown && pinned(nativeMain));
        }
        // Native main window already has the exact native footer; leave it
        // visible without an overlay or replacement artwork. Only other surfaces
        // need an owned navigation view.
        auto hosts=surfaceHosts?surfaceHosts():QList<QWidget *>{};
        if(auto primary=primarySurface?primarySurface():nullptr)if(!hosts.contains(primary))hosts.append(primary);
        for(auto host:hosts) {
            if(!host || host==(root?root():nullptr) || host->property("unboundToolbarStrip").toBool())continue;
            auto layout=qobject_cast<QBoxLayout *>(host->layout());if(!layout || layout->direction()!=QBoxLayout::TopToBottom)continue;
            auto view=views.value(host);
            if(nativeLease.ownerSurface()==host){if(view)view->hide();continue;}
            if(!view) {
                view=new NativeFooterMirror(host);views[host]=view;layout->addWidget(view,0);
                view->activate=[this](QAbstractButton *button){
                    if(!button || !nativeFooter || !nativeFooter->isAncestorOf(button))return;
                    QPointer<QAbstractButton> target=button;
                    logEvent("footer.control.request",{{"target",button->objectName()},{"route","complete-native-footer-mirror"}});
                    QTimer::singleShot(0,this,[this,target]{if(target && target->isEnabled()){
                        const auto name=target->objectName();target->click();schedule();
                        logEvent("footer.control.dispatched",{{"target",name},{"target_alive",bool(target)}});
                    }});
                };
                connect(host,&QObject::destroyed,this,[this,host]{views.remove(host);});
            }
            view->setSource(nativeFooter);view->setVisible(shown && pinned(host) && nativeLease.ownerSurface()!=host && bool(nativeFooter) && !host->property("unboundExpanded").toBool());
        }
        for(auto it=views.begin();it!=views.end();++it)if(it.value() && !hosts.contains(it.key()))it.value()->hide();
        if(nativeCaller)wireCaller(nativeCaller);
        auto primary=primarySurface?primarySurface():nullptr;
        if(placementChanged)placementChanged(bool(primary),pinned(primary));
        if(geometryChanged)geometryChanged();
    }
    void wireCaller(QMenu *menu) {
        auto entry=menu->findChild<QAction *>("unboundPageNavigation",Qt::FindDirectChildrenOnly);
        if(!entry) {
            entry=menu->addAction("Page Navigation");entry->setObjectName("unboundPageNavigation");entry->setProperty("openGateNavigationProxy",true);entry->setCheckable(true);
            connect(entry,&QAction::triggered,this,[this](bool value){defer([this,value]{requestVisibility(value);});});
            connect(menu,&QMenu::aboutToShow,this,[this]{refresh();});
        }
        entry->setChecked(shown);
    }
    PanelAccessoryLease nativeLease;
    std::function<QWidget *()> root;QJsonObject state;QTimer timer;
    QPointer<QWidget> nativeFooter;QPointer<QAction> nativeVisibility;
    QPointer<QMenu> nativeCaller;QHash<QWidget *,QPointer<NativeFooterMirror>> views;
    bool searchedCaller=false;QElapsedTimer actionSearch;
    bool shown=true,pendingVisibility=true,initialized=false,refreshing=false,applyingPlacement=false,stopped=false;
};
}
