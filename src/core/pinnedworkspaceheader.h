// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QScopedValueRollback>
#include "panelaccessorylease.h"
#include "nativerootlayout.h"
namespace Unbound {
// One real native toolbar; its owner and restore slot remain authoritative.
// This controller owns placement and visibility, never panel grouping or keys.
class PinnedWorkspaceHeader : public QObject {
public:
    explicit PinnedWorkspaceHeader(QObject *parent,QJsonObject saved={})
        :QObject(parent),lease(this),visible(saved.value("visible").toBool(true)) {
        timer.setSingleShot(true);timer.setInterval(40);
        connect(&timer,&QTimer::timeout,this,[this]{refresh();});qApp->installEventFilter(this);
    }
    ~PinnedWorkspaceHeader() override {shutdown();}
    std::function<QWidget *()> page,toolbar,primary;
    std::function<QString()> prepareNativeToolbar;
    std::function<void()> changed,geometryChanged;
    QRect externalBounds(QWidget *host) const {
        if(!visible || !observedToolbar || !observedToolbar->isVisible() || observedToolbar->window()==host)return {};
        if(lease.active())return lease.visibleBounds();
        return QRect(observedToolbar->mapToGlobal(QPoint()),observedToolbar->size());
    }
    std::function<void(QString)> failed;
    bool shown() const{return visible;}
    QJsonObject snapshot() const{return {{"model","pinned-top-v1"},{"visible",visible}};}
    void setState(QJsonObject saved){visible=saved.value("visible").toBool(true);schedule();}
    void setShown(bool value){if(visible==value){schedule();return;}logEvent("header.pinned.visibility.request",{{"visible",value}});visible=value;schedule();if(changed)changed();}
    void schedule(){if(!stopped && !timer.isActive())timer.start();}
    void populateMenu(QMenu *menu) {
        auto action=menu->addAction("Workspace Toolbar");action->setObjectName("openGateShowWorkspaceHeader");
        action->setToolTip("Show or hide the pinned workspace controls; Panels remains available");
        action->setCheckable(true);action->setChecked(visible);
        connect(action,&QAction::triggered,this,[this](bool value){QTimer::singleShot(0,this,[this,value]{setShown(value);});});
    }
    void refresh() {
        if(stopped || refreshing || QApplication::activePopupWidget() || QApplication::activeModalWidget())return;
        QScopedValueRollback<bool> guard(refreshing,true);
        auto context=page?page():nullptr;observedPage=context;
        if(!context || !context->isVisible()){if(!lease.release())report("Could not restore workspace header before changing pages.");return;}
        QPointer<QWidget> host=primary?primary():nullptr;
        auto source=toolbar?toolbar():nullptr;
        // A complete embedded layout parks the original toolbar with the
        // native page tree. Same native window does not mean a visible layout slot.
        if(host && host->window()==context->window() && source &&
           (NativeRootLayout::ownsParkedWidget(source,context) || (lease.active()&&lease.ownerSurface()==context)))host=context;
        if(!host || host->window()==context->window()) {
            if(host==context && (lease.active() || (source&&NativeRootLayout::ownsParkedWidget(source,context)))) {
                if(!lease.active()) {
                    const auto error=lease.attach(source,context->window(),context,Qt::TopEdge,true,true);
                    if(!error.isEmpty()){report(error);return;}
                    observedToolbar=source;
                }
                lease.setHidden(!visible);if(geometryChanged)geometryChanged();return;
            }
            if(!lease.release()){report("The workspace header's original slot changed.");return;}
            observedToolbar=toolbar?toolbar():nullptr;
            if(observedToolbar)observedToolbar->setVisible(visible);
            if(geometryChanged)geometryChanged();
            return;
        }
        if(lease.active() && lease.ownerSurface()!=host && !lease.release()){report("Could not transfer workspace header to the new surface.");return;}
        if(!lease.active()) {
            if(prepareNativeToolbar){const auto error=prepareNativeToolbar();if(!error.isEmpty()){report(error);return;}}
            if(!host){schedule();return;}
            auto source=toolbar?toolbar():nullptr;observedToolbar=source;
            if(!source)return;
            // Resolve's main toolbar is a sibling of the Edit page. Acquire
            // from the original main-window tree, never from an unrelated page.
            source->show();
            const auto error=lease.attach(source,context->window(),host,Qt::TopEdge);
            if(!error.isEmpty()){report(error);return;}
        }
        lease.setHidden(!visible);if(geometryChanged)geometryChanged();
    }
    bool release(){return lease.release();}
    void shutdown(){if(stopped)return;stopped=true;timer.stop();qApp->removeEventFilter(this);lease.release();}
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(stopped)return false;
        if(event->type()==QEvent::Show || event->type()==QEvent::Hide || event->type()==QEvent::LayoutRequest || event->type()==QEvent::ParentChange) {
            auto widget=qobject_cast<QWidget *>(object);
            if(widget && (widget->isWindow() || widget==observedPage || widget->objectName().startsWith("WStackPage_")))schedule();
        }
        return false;
    }
private:
    void report(QString error){logEvent("header.pinned.failed",{{"error",error}});if(failed)failed(error);}
    QPointer<QWidget> observedPage,observedToolbar;
    PanelAccessoryLease lease;QTimer timer;bool visible=true,refreshing=false,stopped=false;
};
}
