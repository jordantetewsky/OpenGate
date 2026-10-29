// SPDX-License-Identifier: MIT
#pragma once
#include "panelexpansion.h"
#include "panelexpansionconstraints.h"
#include "panelsession.h"
#include "panelgroup.h"
#include "surfacereadiness.h"
#include "workspacesettings.h"
#include <QWindow>

namespace Unbound {
// Owns one reversible expansion transaction, including readiness cancellation
// and original presentation. Manager supplies transitions, not internal state.
class PanelExpansionController : public QObject {
public:
    explicit PanelExpansionController(QObject *owner) : QObject(owner) {}
    struct Target {
        int index=-1;
        QString name;
        ManagedPanelSession *session=nullptr;
        QPointer<QWidget> workspace;
        PanelGroup *group=nullptr;
        QJsonObject saved,native;
        std::function<bool()> pending;
        std::function<void()> undock,cancelPending,restored;
        std::function<void(QJsonObject)> restoreSaved;
        std::function<QString(QJsonObject)> restoreNative;
    };
    std::function<bool()> running;
    std::function<void(int)> update;
    std::function<void()> save,presentationChanged;
    std::function<void(QString)> report;
    std::function<QRect(QWidget *)> contentBounds;
    QWidget *workspace() const {return current>=0?target.workspace.data():nullptr;}
    bool refreshGeometry() {
        auto window=current>=0?host():nullptr;if(!window)return true;
        // A standalone primary surface already fills its own workspace. Its
        // layout expands/reclaims content when bands are hidden or shown; never
        // resize that window into its own child rectangle (recursive shrinking).
        if(target.workspace==window)return true;
        const auto margins=window->windowHandle()?window->windowHandle()->frameMargins():QMargins();
        QRect bounds;
        if(contentBounds) {
            if(!target.workspace){if(report)report(QStringLiteral("The original workspace is no longer available."));return false;}
            bounds=contentBounds(target.workspace);
        } else {
            // Legacy standalone controller fixtures do not supply a workspace.
            auto screen=window->screen();if(!screen)screen=QApplication::primaryScreen();
            if(screen)bounds=screen->availableGeometry();
        }
        if(!bounds.isEmpty())bounds=bounds.adjusted(margins.left(),margins.top(),-margins.right(),-margins.bottom());
        if(bounds.isEmpty()){report(QStringLiteral("The workspace content area is unavailable for this panel."));return false;}
        if(window->geometry()!=bounds) {
            window->setGeometry(bounds);
            logEvent("panel.expand.bounds",{{"panel",target.name},{"x",bounds.x()},{"y",bounds.y()},{"width",bounds.width()},{"height",bounds.height()},{"scope",contentBounds?"workspace-viewport":"legacy-screen"}});
        }
        return true;
    }
    int index() const { return current; }
    void sessionInactive(int index) { if (current==index) {constraints.restore();priority.restore();current=-1;} }
    QJsonObject panelSnapshot(int index,const QJsonObject &value) const { return current==index ? target.saved : value; }
    QJsonObject groupSnapshot(PanelGroup *group) const
    { return current>=0 && group==originalGroup ? originalGroupState : group->snapshot(); }
    QJsonObject nativeSnapshot(const QJsonObject &value) const
    { return current>=0 && !target.native.isEmpty() ? target.native : value; }
    void begin(Target value,QWidget *page)
    {
        target=std::move(value);
        originalOffset=target.session->overflowOffset();
        wasFloating=target.session->active(); wasGrouped=target.session->grouped();
        originalGroup=target.group;
        originalGroupState=originalGroup ? originalGroup->snapshot() : QJsonObject();
        if (wasGrouped && !target.session->beginGroupedExpansion()) return;
        current=target.index; const unsigned request=++version;
        if (!wasFloating) target.undock();
        new SurfaceReadiness(this,page,
            [this,request] { return current==target.index && request==version && running(); },
            [this] { return target.session->active() && !target.pending() && host(); },
            [this,request](bool ready) {
                if (current!=target.index || request!=version) return;
                auto window=host();
                if (!ready || !window) {
                    logEvent("panel.expand.failed",{{"panel",target.name},{"reason","surface-not-ready"}});
                    restore(); report(QStringLiteral("Could not expand the panel; its previous arrangement was restored."));
                    return;
                }
                constraints.acquire(target.name,window);
                if(!refreshGeometry()){restore();return;}
                target.session->setHiddenByUser(false); priority.foreground(window); update(target.index);
                if(presentationChanged)presentationChanged();
                logEvent("panel.expand",{{"panel",target.name},{"originally_floating",wasFloating}});
            });
        update(target.index);
    }
    void restore()
    {
        if (current<0) return;
        const int index=current; constraints.restore();priority.restore(); ++version; current=-1;
        target.cancelPending(); QString error;
        bool groupRebuilt=false;
        if (wasGrouped) {
            target.session->endGroupedExpansion();
            // Borrowing one leaf's content does not necessarily change its
            // retained group. Rebuilding an identical tree reparents every leaf
            // and recreates tabs/splitters on each maximize/restore pair.
            if (originalGroup && originalGroup->active() && originalGroup->snapshot()!=originalGroupState)
                groupRebuilt=originalGroup->restore(originalGroupState);
        }
        originalGroup=nullptr;
        if (target.session->active()) {
            target.session->setFloatingGeometry(WorkspaceSettings::rectangle(target.saved));
            target.session->setHiddenByUser(target.saved["hidden"].toBool());
            if (!wasFloating) error=target.session->dock();
        }
        target.restoreSaved(target.saved);
        if (!target.native.isEmpty()) {
            const auto nativeError=target.restoreNative(target.native);
            if (!nativeError.isEmpty()) error=nativeError;
            target.native={};
        }
        target.session->restoreOverflowOffset(originalOffset);
        target.restored();
        update(index); save();if(presentationChanged)presentationChanged();
        logEvent("panel.expand.restore",{{"panel",target.name},{"success",error.isEmpty()},{"error",error},{"group_rebuilt",groupRebuilt}});
        if (!error.isEmpty()) report(error);
    }
private:
    QWidget *host() const { return wasGrouped ? target.session->expandedWindow() : target.session->window(); }
    Target target;
    int current=-1;
    unsigned version=0;
    bool wasFloating=false,wasGrouped=false;
    QPoint originalOffset;
    QPointer<PanelGroup> originalGroup;
    QJsonObject originalGroupState;
    PanelExpansion priority;
    PanelExpansionConstraints constraints;
};
}
