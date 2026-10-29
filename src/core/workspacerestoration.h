// SPDX-License-Identifier: MIT
#pragma once
#include "nativeworkspacebinding.h"
#include "workspacesettings.h"

namespace Unbound {
// Workspace restore sequencing only. UI preference application and individual
// panel transitions remain explicit callbacks owned by their respective layers.
class WorkspaceRestoration : public QObject {
public:
    explicit WorkspaceRestoration(QObject *owner) : QObject(owner) {}
    struct Panel { QString name; QJsonObject saved; bool desired=false; ManagedPanelSession *session=nullptr; };
    struct ResumeResult { QString error; bool prepared=false; };
    std::function<bool()> running;
    std::function<QJsonObject()> checkpoint;
    std::function<QString(const QJsonObject &)> validate,apply;
    std::function<QString()> dock;
    std::function<void()> resume;
    std::function<QStringList()> unavailable;
    std::function<bool()> groupsPending;
    std::function<void(QString)> report;
    std::function<void()> committed;
    bool active() const { return !restoringId.isEmpty(); }
    bool savingBlocked() const { return active() || checkpointBlocked; }

    void preset(const QJsonObject &entry,bool pageReady)
    {
        logEvent("button.click",{{"action","arrangement-restore"},{"panel",""}});
        if (entry.isEmpty()) { report("Select a saved arrangement first."); return; }
        if (!pageReady) { report("Open the Edit page to restore an arrangement."); return; }
        const auto value=entry["workspace"].toObject();
        const auto validation=validate(value);
        if (!validation.isEmpty()) {
            report(validation); logEvent("workspace.native-plan.failed",{{"error",validation},{"stage","preset-preflight"}}); return;
        }
        const auto fallback=checkpoint?checkpoint():QJsonObject();
        cancel();
        previous=fallback;
        restoringId=entry["id"].toString();
        const auto blocked=dock();
        if (!blocked.isEmpty()) {
            report("Could not restore: a panel could not be docked safely.");
            logEvent("arrangement.restore.failed",{{"panel",blocked}}); cancel();checkpointBlocked=true; return;
        }
        const auto error=apply(value);
        if (!error.isEmpty()) { failPreset(error); return; }
        logEvent("arrangement.restore.request",{{"id",restoringId}});
        resume();
        finishPreset();
    }
    ResumeResult native(QWidget *root,NativeWorkspaceBinding *binding,const QList<Panel> &panels,
                        const std::function<void(int)> &request)
    {
        const auto token=++generation;
        const auto error=binding->preparePending(root);
        if (!error.isEmpty()) return {error,false};
        const auto anchor=binding->pendingFloatingAnchor();
        if (!anchor.isEmpty()) for (const auto &panel:panels) if (panel.name==anchor && panel.session->active()) {
            const auto geometry=WorkspaceSettings::rectangle(panel.saved);
            if (geometry.isValid()) panel.session->setFloatingGeometry(geometry);
            panel.session->setPinned(panel.saved["pinned"].toBool());
            panel.session->setHiddenByUser(panel.saved["hidden"].toBool(),false);
        }
        // Native panel toggles can be mutually exclusive (Media Pool / Index).
        // Acquire one surface completely before asking Resolve to reveal another.
        for (int i=0;i<panels.size();++i) if (panels[i].desired && !panels[i].session->active()) { request(i); break; }
        // A synchronous acquisition failure may already have started rollback.
        // Never mount or finish the superseded request after that callback.
        if(token!=generation)return {{},false};
        const auto nativeError=binding->restoreReady(root); binding->refreshNativeTargets(root);
        return {nativeError,true};
    }
    void settled(bool groups,bool background,std::function<void()> restoreGroups,std::function<void()> hideBackground)
    {
        const auto token=++generation;
        if (groups) QTimer::singleShot(120,this,[this,token,restoreGroups] {
            if (token==generation && running()) restoreGroups();
        });
        if (background) QTimer::singleShot(200,this,[this,token,hideBackground] {
            if (token==generation && running()) hideBackground();
        });
    }
    bool failPreset(const QString &error) {
        if(!active())return false;
        ++generation;
        if(rollingBack) {
            const auto id=restoringId;
            const auto cleanup=dock();
            report(QStringLiteral("Could not restore the previous workspace: ")+error);
            logEvent("arrangement.restore.rollback.failed",{{"id",id},{"error",error},{"cleanup_error",cleanup}});
            cancel();checkpointBlocked=true;return true;
        }
        failure=error;rollingBack=true;
        const auto dockError=dock();
        const auto applyError=dockError.isEmpty()&&!previous.isEmpty()?apply(previous):QString();
        logEvent("arrangement.restore.failed",{{"id",restoringId},{"error",error},{"rollback_started",dockError.isEmpty()&&applyError.isEmpty()&&!previous.isEmpty()}});
        if(!dockError.isEmpty() || !applyError.isEmpty() || previous.isEmpty()) {
            report(QStringLiteral("Could not restore arrangement: ")+error);
            logEvent("arrangement.restore.rollback.failed",{{"id",restoringId},{"error",dockError+applyError}});
            cancel();checkpointBlocked=true;return true;
        }
        report(QStringLiteral("Could not restore arrangement: ")+error+" Restoring previous workspace…");
        resume();finishPreset();
        return true;
    }
    void finishPreset() {
        if(!active())return;
        const auto missing=unavailable();
        if(groupsPending()) { report(rollingBack?QStringLiteral("Could not restore arrangement: ")+failure+" Restoring previous workspace…":QStringLiteral("Restoring arrangement…"));return; }
        if(!missing.isEmpty()) { report(QStringLiteral("Unavailable: ")+missing.join(", "));return; }
        if(rollingBack) {
            report(QStringLiteral("Could not restore arrangement: ")+failure+" Previous workspace restored.");
            logEvent("arrangement.restore.rollback",{{"id",restoringId},{"complete",true}});
        } else {
            report("Arrangement restored.");
            logEvent("arrangement.restore",{{"id",restoringId},{"unavailable",""},{"complete",true}});
        }
        cancel();if(committed)committed();
    }
    void cancel() { ++generation;restoringId.clear();previous={};rollingBack=false;failure.clear();checkpointBlocked=false; }

private:
    unsigned generation=0;
    QString restoringId,failure;
    bool rollingBack=false,checkpointBlocked=false;
    QJsonObject previous;
};
}
