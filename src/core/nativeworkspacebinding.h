// SPDX-License-Identifier: MIT
#pragma once
#include "nativearrangement.h"
#include "panelgroup.h"
#include "toolbars.h"
namespace Unbound {
// Bind session commands and saved workspace state to the page coordinator. UI
// controls and native menus call this same route; it owns no window chrome.
class NativeWorkspaceBinding : public QObject {
public:
    explicit NativeWorkspaceBinding(QObject *owner) : QObject(owner),arrangement(this) {
        arrangement.restored=[this] { if (changed) changed(); };
        arrangement.prepareRestore=[this]{return prepareMount?prepareMount():QString();};
        arrangement.detachRequested=[this](const QString &id){detach(id);};
        arrangement.tearOff=[this](const QString &id,QPoint pointer) {
            QPointer<ManagedPanelSession> session=registry.value(id);
            if (!detach(id) || !session || !session->floating()) return;
            auto floating=static_cast<WorkspaceFloatingHost *>(session->window());
            floating->move(pointer-QPoint(80,12)); floating->raise(); floating->activateWindow();
            floating->beginPointerDrag(pointer,true);
        };
        arrangement.tearOffSection=[this](const QJsonObject &tree,QRect frame,QPoint pointer) {
            const auto error=detachSection(tree,frame,pointer);
            if (!error.isEmpty() && failed) failed(error);
        };
    }
    ~NativeWorkspaceBinding() override { if (nativeTargetPage) unregisterNativeDockSurfaces(nativeTargetPage); }
    std::function<void()> changed;
    std::function<void()> beforeMount;
    std::function<QString()> prepareMount;
    std::function<void(const QString &)> failed;
    std::function<void(PanelGroup *)> sectionDetached;
    QString detachSection(const QJsonObject &tree,QRect frame,QPoint pointer) {
        UiWorkTiming timing("workspace.native-section.transfer",arrangement.surface() ? arrangement.surface()->window() : nullptr);
        QStringList names;
        if (busy || !active() || !groupTreeNames(tree,names) || names.size()<2 || names.contains(nativeRemainderId()))
            return QStringLiteral("Choose a retained panel section to detach.");
        QList<ManagedPanelSession *> members;
        for (const auto &id : names) {
            auto session=registry.value(id);
            if (!session || !contains(id)) return QStringLiteral("The native section changed before detachment.");
            members.append(session);
        }
        QScopedValueRollback<bool> guard(busy,true);
        const auto previous=arrangement.savedPlan("edit");
        QPointer<QWidget> page=members.first()->pageWidget();
        const auto rollback=[&](const QString &error) {
            if (!arrangement.restore(false)) return error+QStringLiteral(" Native restoration requires recovery.");
            const auto anchor=previous.value("anchor").toString();
            if (!anchor.isEmpty()) {
                auto session=registry.value(anchor);
                if (!session || session->grouped()) return error+QStringLiteral(" The native anchor is unavailable for recovery.");
                if (session->active()) {
                    const auto dockError=session->dock();
                    if (!dockError.isEmpty()) return error+QStringLiteral(" ")+dockError;
                }
                const auto prepareError=arrangement.preparePosition(panelSurface(page,anchor),page,session,anchor,anchor);
                if (!prepareError.isEmpty()) return error+QStringLiteral(" ")+prepareError;
            }
            QHash<QString,ManagedPanelSession *> original;
            for (const auto &id : previous["panels"].toObject().keys()) if (registry.value(id)) original[id]=registry.value(id);
            const auto restored=arrangement.initializeSaved(page,original,previous,"edit");
            logEvent("workspace.native-section.rollback",{{"error",error},{"rollback_error",restored}});
            return restored.isEmpty() ? error : error+QStringLiteral(" ")+restored;
        };
        for (const auto &id : names) if (!arrangement.detach(id,true)) return rollback(QStringLiteral("Could not release the native section."));
        auto group=new PanelGroup(parent());
        const auto error=group->initialize(members,tree,frame);
        if (!error.isEmpty()) { group->dissolve(false,true); delete group; return rollback(error); }
        if (sectionDetached) sectionDetached(group);
        auto floating=static_cast<WorkspaceFloatingHost *>(group->window());
        floating->move(pointer-QPoint(80,12)); floating->raise(); floating->activateWindow();
        floating->beginPointerDrag(pointer,true);
        logEvent("workspace.native-section.detach",{{"panels",QJsonArray::fromStringList(names)},{"x",pointer.x()},{"y",pointer.y()}});
        if (changed) changed();
        return {};
    }
    void registerPanel(const QString &id,ManagedPanelSession *session,const QString &title={}) {
        registry[id]=session; titles[id]=title.isEmpty() ? id : title;
    }
    void refreshNativeTargets(QWidget *page) {
        if (busy) return;
        if (nativeTargetPage) unregisterNativeDockSurfaces(nativeTargetPage);
        nativeTargets.clear(); nativeTargetPage=page;
        if (!page || !page->isVisible() || active() || arrangement.positionPrepared()) return;
        QList<QWidget *> surfaces;
        for (auto it=registry.begin();it!=registry.end();++it) if (it.value() && !it.value()->active()) {
            if (it.key()=="frameEditToolbarTop" || it.key()=="unboundWorkspaceToolbar") continue;
            auto surface=panelSurface(page,it.key());
            if (NativePanelPosition::supported(surface,page)) { nativeTargets[it.key()]=surface; surfaces.append(surface); }
        }
        if (!surfaces.isEmpty()) registerNativeDockSurfaces(page->window(),page,surfaces);
    }
    QString targetName(QWidget *target) const {
        for (auto it=nativeTargets.begin();it!=nativeTargets.end();++it) if (it.value()==target) return it.key();
        return dockTargetKey(target);
    }
    bool contains(const QString &id) const { return arrangement.contains(id); }
    bool selected(const QString &id) const { return arrangement.selected(id); }
    bool active() const { return arrangement.active(); }
    bool ownsTarget(QWidget *target) const {
        for (auto it=nativeTargets.begin();it!=nativeTargets.end();++it) if (it.value()==target && it.value() && it.value()->isVisible()) return true;
        auto surface=arrangement.surface();
        return surface && target && surface->isAncestorOf(target) && !dockTargetKey(target).isEmpty();
    }
    QString validate(const QJsonObject &saved,const QJsonArray &floatingGroups={}) const {
        if (saved.isEmpty()) return {};
        QSet<QString> names; for (auto it=registry.begin();it!=registry.end();++it) names.insert(it.key());
        NativeArrangementPlan plan; const auto error=NativeArrangementPlan::parse(saved,"edit",names,plan);
        if (!error.isEmpty()) return error;
        const auto states=saved["panels"].toObject();
        for (const auto &group : floatingGroups) {
            QStringList members; groupTreeNames(PanelGroup::savedTree(group.toObject()),members);
            for (const auto &id : members) if (states.contains(id)) return QStringLiteral("A saved panel belongs to both native and floating groups.");
        }
        return {};
    }
    QString setPending(const QJsonObject &saved,const QJsonArray &floatingGroups={}) {
        const auto error=validate(saved,floatingGroups);
        if (error.isEmpty()) pending=saved; else logEvent("workspace.native-plan.failed",{{"error",error}});
        return error;
    }
    QStringList requiredPanels() const {
        auto result=pending["panels"].toObject().keys(); const auto anchor=pending["anchor"].toString();
        if (!anchor.isEmpty() && !result.contains(anchor)) result.append(anchor);
        return result;
    }
    QString preparePending(QWidget *page) {
        const auto anchor=pending.value("anchor").toString();
        if (busy || anchor.isEmpty() || active() || arrangement.positionPrepared()) return {};
        auto session=registry.value(anchor);
        if (!page || !page->isVisible() || !session || session->active()) return QStringLiteral("The saved native anchor is unavailable.");
        auto surface=panelSurface(page,anchor);
        if (!NativePanelPosition::supported(surface,page)) return QStringLiteral("The saved native anchor position is unavailable.");
        QScopedValueRollback<bool> guard(busy,true);
        if(prepareMount){const auto error=prepareMount();if(!error.isEmpty())return error;}
        if (beforeMount) beforeMount();
        return arrangement.preparePosition(surface,page,session,anchor,titles.value(anchor,anchor));
    }
    QString pendingFloatingAnchor() const {
        const auto anchor=pending.value("anchor").toString();
        return pending.value("panels").toObject().contains(anchor) ? QString() : anchor;
    }
    void queueRestore(QWidget *page) {
        if (queued || pending.isEmpty() || active()) return;
        queued=true; QPointer<QWidget> safePage=page;
        QTimer::singleShot(0,this,[this,safePage] {
            queued=false; const auto error=restoreReady(safePage);
            if (!error.isEmpty() && failed) failed(error);
        });
    }
    QString restoreReady(QWidget *page) {
        if (busy || pending.isEmpty() || active() || !page || !page->isVisible()) return {};
        QHash<QString,ManagedPanelSession *> sessions;
        const auto states=pending["panels"].toObject();
        for (auto it=states.begin();it!=states.end();++it) {
            auto session=registry.value(it.key());
            if (!session || !session->active() || session->grouped()) return {};
            sessions[it.key()]=session;
        }
        QScopedValueRollback<bool> guard(busy,true);
        logEvent("workspace.native-plan.restore.request",{{"page","edit"},{"panels",sessions.size()}});
        if(prepareMount){const auto error=prepareMount();if(!error.isEmpty())return error;}
        if (beforeMount) beforeMount();
        const auto error=arrangement.initializeSaved(page,sessions,pending,"edit");
        if (error.isEmpty()) { pending={}; if (changed) changed(); }
        else logEvent("workspace.native-plan.restore.failed",{{"error",error},{"plan",pending}});
        return error;
    }
    QJsonObject saved() const { return active() ? arrangement.savedPlan("edit") : pending; }
    QString defaultDockTarget() const {
        if(!active())return nativeRemainderId();
        QStringList names;groupTreeNames(arrangement.snapshot(),names);
        if(names.contains(nativeRemainderId()))return nativeRemainderId();
        if(names.contains("frameConformTimelines"))return QStringLiteral("frameConformTimelines");
        return names.value(0,nativeRemainderId());
    }
    QString restorePresentation(const QJsonObject &saved) {
        if (busy) return QStringLiteral("The native workspace is changing.");
        QScopedValueRollback<bool> guard(busy,true); const auto error=arrangement.restorePresentation(saved);
        if (error.isEmpty() && changed) changed(); return error;
    }
    QString dock(QWidget *page,const QHash<QString,ManagedPanelSession *> &sessions,const QJsonObject &tree,
        const QString &target,Qt::Orientation direction,bool before) {
        if (busy) return QStringLiteral("The native workspace is changing.");
        if (!active() && target!=nativeRemainderId()) return QStringLiteral("The native drop target is unavailable.");
        QScopedValueRollback<bool> guard(busy,true);
        if(!active() && prepareMount){const auto error=prepareMount();if(!error.isEmpty())return error;}
        if (!active() && beforeMount) beforeMount();
        const auto error=active() ? arrangement.add(sessions,tree,target,direction,before) : arrangement.initialize(page,sessions,
            splitGroupTree({{"panel",nativeRemainderId()}},tree,direction,before));
        if (error.isEmpty()) { pending={}; if (changed) changed(); }
        return error;
    }
    QString dockSelection(QWidget *page,const QString &id,PanelGroup *group,const QString &target,
        Qt::Orientation direction,bool before) {
        if (busy || !page || !page->isVisible()) return QStringLiteral("Open the native page before docking here.");
        auto selected=registry.value(id);
        if (!selected || !selected->active() || contains(id)) return QStringLiteral("Choose a floating panel or group.");
        QHash<QString,ManagedPanelSession *> panels;
        const auto previous=group ? group->snapshot() : QJsonObject();
        const auto tree=group ? PanelGroup::savedTree(previous) : QJsonObject{{"panel",id}};
        for (auto it=registry.begin();it!=registry.end();++it)
            if (it.value() && (group ? group->contains(it.value()) : it.key()==id)) panels[it.key()]=it.value();
        const auto sourcePanels=panels;
        const bool initial=!active() && target!=nativeRemainderId() && nativeTargets.value(target);
        if (initial) {
            QScopedValueRollback<bool> guard(busy,true);
            auto anchor=registry.value(target);
            if (!anchor || anchor->active()) return QStringLiteral("The native drop target changed.");
            if (!NativePanelPosition::supported(nativeTargets.value(target),page)) return QStringLiteral("The native drop position changed.");
            logEvent("workspace.native-position.drop.request",{{"source",id},{"anchor",target},{"members",panels.size()},
                {"orientation",int(direction)},{"before",before}});
            if(prepareMount){const auto error=prepareMount();if(!error.isEmpty())return error;}
        if (beforeMount) beforeMount();
            auto error=arrangement.preparePosition(nativeTargets.value(target),page,anchor,target,titles.value(target,target));
            if (!error.isEmpty()) return error;
            if (group) group->dissolve(false,true);
            panels[target]=anchor;
            auto scopedTree=splitGroupTree({{"panel",target}},tree,direction,before);
            if (direction==Qt::Orientation(0)) scopedTree["selected"]=before ? 0 : 1;
            error=arrangement.initialize(page,panels,scopedTree);
            if (!error.isEmpty()) {
                const bool released=arrangement.restore(false);
                if (released && anchor->active()) {
                    const auto dockError=anchor->dock();
                    if (!dockError.isEmpty()) error+=QStringLiteral(" ")+dockError;
                }
                if (group) {
                    QList<ManagedPanelSession *> members; for (auto session : sourcePanels) members.append(session);
                    const auto rollback=group->initialize(members,tree,WorkspaceSettings::rectangle(previous));
                    if (rollback.isEmpty()) group->restore(previous);
                    else error+=QStringLiteral(" ")+rollback;
                }
                if (!released) error+=QStringLiteral(" Native position recovery is incomplete.");
            } else { pending={}; if (changed) changed(); }
            logEvent("workspace.native-position.drop",{{"source",id},{"anchor",target},{"success",error.isEmpty()},{"error",error}});
            return error;
        }
        logEvent("workspace.native-transfer.request",{{"panel",id},{"members",panels.size()},{"target",target},{"orientation",int(direction)}});
        if (group) group->dissolve(false,true);
        const auto error=dock(page,panels,tree,target,direction,before);
        if (!error.isEmpty() && group) {
            QList<ManagedPanelSession *> members; for (auto session : panels) members.append(session);
            const auto rollback=group->initialize(members,tree,WorkspaceSettings::rectangle(previous));
            if (rollback.isEmpty()) group->restore(previous);
            else return error+QStringLiteral(" Original group could not be rebuilt: ")+rollback;
        }
        return error;
    }
    bool setVisible(const QString &id,bool visible) {
        if (busy) return false; QScopedValueRollback<bool> guard(busy,true);
        const bool result=arrangement.setPanelVisible(id,visible); if (result && changed) changed(); return result;
    }
    bool detach(const QString &id) {
        if (busy) return false; QScopedValueRollback<bool> guard(busy,true);
        const bool result=arrangement.detach(id);
        if (result) if (auto session=registry.value(id)) if (session->floating()) session->setHiddenByUser(false);
        if (result && changed) changed(); return result;
    }
    bool release(bool dockPanels = false) {
        if (busy) return false; QScopedValueRollback<bool> guard(busy,true);
        pending={}; const bool result=arrangement.restore(dockPanels);
        if (!result) logEvent("workspace.native-recovery.failed");
        if (result && changed) changed(); return result;
    }
private:
    NativeArrangement arrangement;
    QHash<QString,QPointer<ManagedPanelSession>> registry;
    QHash<QString,QString> titles;
    QHash<QString,QPointer<QWidget>> nativeTargets;
    QPointer<QWidget> nativeTargetPage;
    QJsonObject pending;
    bool busy=false,queued=false;
};
}
