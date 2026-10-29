// SPDX-License-Identifier: MIT
#pragma once
#include <QHash>
#include "grouplayout.h"
#include "nativerootlayout.h"
#include "panellayoutsnapshot.h"
#include "panellayoutsurface.h"
#include "panellayoutproportions.h"
#include "panelsession.h"
#include "nativedocksurfaces.h"
#include "nativearrangementplan.h"
#include "panellayoutvisibility.h"
#include "grouppanelheader.h"
#include "paneltitlegesture.h"
namespace Unbound {
// Page-level ownership, independent of floating host chrome and drop detection.
// The retained native layout is always represented explicitly in the tree.
class NativeArrangement : public QObject {
public:
    explicit NativeArrangement(QObject *owner=nullptr) : QObject(owner),lease(this) {}
    ~NativeArrangement() override {
        if (!restore(false)) for (auto session : sessions) if (session) {
            session->releaseGroup={}; session->detachFromGroup();
        }
        if (anchorSession) anchorSession->releaseOriginalPosition={};
    }
    static QString nativeLeaf() { return nativeRemainderId(); }
    bool active() const { return lease.active() || scopedLease.active(); }
    bool positionPrepared() const { return position.active(); }
    QString preparePosition(QWidget *panel,QWidget *page,ManagedPanelSession *session,const QString &id,const QString &title) {
        if (changing || host || position.active() || !session || session->active() || session->releaseOriginalPosition)
            return QStringLiteral("The native panel position is already in use.");
        auto error=position.acquire(panel,page); if (!error.isEmpty()) return error;
        error=session->undock(panel,page,title,true,true);
        if (!error.isEmpty()) { position.restore(); return error; }
        anchorSession=session; anchorId=id;
        session->releaseOriginalPosition=[this] {
            const bool recovered=restore(false);
            if (recovered && restored) restored();
            return recovered ? QString() : QStringLiteral("Could not release the native arrangement position safely.");
        };
        return {};
    }
    QWidget *surface() const { return host; }
    std::function<void(const QString &,QPoint)> tearOff;
    std::function<void(const QString &)> detachRequested;
    std::function<void()> restored;
    std::function<QString()> prepareRestore;
    std::function<void(const QJsonObject &,QRect,QPoint)> tearOffSection;
    bool contains(const QString &id) const { return active() && sessions.value(id); }
    bool selected(const QString &id) const {
        auto session=sessions.value(id); auto cell=cells.value(id);
        return active() && session && cell && !session->hiddenByUser() && !panelCoveredByTab(cell);
    }
    QJsonObject snapshot() const { return panelLayoutSnapshot(root); }
    QJsonObject savedPlan(const QString &pageKey) const {
        if (!active()) return {};
        NativeArrangementPlan plan; plan.page=pageKey; plan.tree=snapshot(); plan.anchor=anchorId; plan.managed=anchorId.isEmpty() && !cells.contains(nativeLeaf());
        for (auto it=sessions.begin();it!=sessions.end();++it) if (it.value() && it.value()->hiddenByUser()) plan.hidden.insert(it.key());
        return plan.json();
    }
    QString restorePresentation(const QJsonObject &saved) {
        if (changing || !active()) return QStringLiteral("The native workspace is not available.");
        QSet<QString> available; for (auto it=sessions.begin();it!=sessions.end();++it) available.insert(it.key());
        if (!anchorId.isEmpty()) available.insert(anchorId);
        NativeArrangementPlan plan; const auto error=NativeArrangementPlan::parse(saved,"edit",available,plan);
        if (!error.isEmpty() || plan.anchor!=anchorId || saved["panels"].toObject().size()!=sessions.size()) return error.isEmpty() ? QStringLiteral("The native panel set changed.") : error;
        QScopedValueRollback<bool> guard(changing,true); UiWorkTiming timing("workspace.native-arrangement.presentation",host->window());
        for (auto it=sessions.begin();it!=sessions.end();++it) if (it.value()) it.value()->setHiddenByUser(plan.hidden.contains(it.key()),false);
        return rebuild(plan.tree) ? QString() : QStringLiteral("Could not restore native tab selection and proportions.");
    }
    QString initializeSaved(QWidget *page,const QHash<QString,ManagedPanelSession *> &panels,const QJsonObject &saved,const QString &pageKey) {
        QSet<QString> available; for (auto it=panels.begin();it!=panels.end();++it) available.insert(it.key());
        if (!anchorId.isEmpty()) available.insert(anchorId);
        NativeArrangementPlan plan; auto error=NativeArrangementPlan::parse(saved,pageKey,available,plan);
        if (!error.isEmpty()) return error;
        if (plan.anchor!=anchorId) return QStringLiteral("The saved native anchor is not prepared.");
        error=initialize(page,panels,plan.tree); if (!error.isEmpty()) return error;
        QScopedValueRollback<bool> guard(changing,true);
        for (auto it=panels.begin();it!=panels.end();++it) {
            if (!active()) return QStringLiteral("The native page changed during restoration.");
            it.value()->setHiddenByUser(plan.hidden.contains(it.key()),false);
        }
        if (!rebuild(plan.tree)) { restoreOwned(false); return QStringLiteral("Could not restore the saved native arrangement."); }
        logEvent("workspace.native-arrangement.restore-plan",{{"page",pageKey}});
        return {};
    }
    bool setPanelVisible(const QString &id,bool visible) {
        auto session=sessions.value(id); auto cell=cells.value(id);
        if (changing || !active() || !root || !session || !cell) return false;
        QScopedValueRollback<bool> guard(changing,true);
        UiWorkTiming timing("workspace.native-arrangement.visibility",host->window());
        visibility.remember(root);
        if (!visible) visibility.closePath(cell,host);
        session->setHiddenByUser(!visible,false);
        if (!active() || !root || !host || !cell) return false;
        syncCellVisibility();
        if (visible) visibility.selectPath(cell,host);
        host->layout()->activate(); visibility.restore(root);
        logEvent("workspace.native-arrangement.visibility",{{"panel",id},{"visible",visible}});
        return true;
    }
    QString initialize(QWidget *page,const QHash<QString,ManagedPanelSession *> &panels,const QJsonObject &tree) {
        QStringList names;
        const bool scoped=position.active();
        if (changing || host || !page || !page->isVisible() || panels.isEmpty() || panels.size()>31 ||
            !groupTreeNames(tree,names) || names.size()!=panels.size()+(names.contains(nativeLeaf()) ? 1 : 0) || (scoped && names.contains(nativeLeaf())))
            return QStringLiteral("Invalid native arrangement.");
        QSet<ManagedPanelSession *> unique;
        for (auto it=panels.begin();it!=panels.end();++it) {
            auto session=it.value();
            if (it.key()==nativeLeaf() || !names.contains(it.key()) || !session || !session->active() || session->grouped() ||
                session->pageWidget()!=page || session->nativeOwnerWindow()!=page->window() || unique.contains(session))
                return QStringLiteral("Choose available panels from this native page.");
            unique.insert(session);
        }
        QScopedValueRollback<bool> guard(changing,true);
        UiWorkTiming timing("workspace.native-arrangement.initialize",page->window());
        host=new QWidget(page->window()); host->setObjectName("unboundNativeArrangement");
        auto layout=new QVBoxLayout(host); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
        for (const auto &name : names) {
            auto cell=new QWidget(host); cell->setObjectName("unboundGroupCell_"+name); cell->setProperty("groupPanel",name);
            if (auto session=panels.value(name)) cell->setWindowTitle(session->panelTitle());
            cells[name]=cell;
        }
        if (!rebuild(tree)) { retire(); return QStringLiteral("Could not render the native arrangement."); }
        auto error=scoped ? scopedLease.mount(position.surface(),host,page->window()) : lease.mount(page,host);
        if (!error.isEmpty()) { if (!lease.active() && !scopedLease.active()) retire(); return error; }
        pageOwner=page;
        ownerDestroyed=connect(page,&QObject::destroyed,this,[this] {
            unregisterNativeDockSurfaces(host);
            for (auto session : sessions) if (session) session->releaseGroup={};
            sessions.clear(); cells.clear(); host=nullptr; root=nullptr; pageOwner=nullptr; visibility.reset();
            if (anchorSession) anchorSession->releaseOriginalPosition={};
            anchorSession=nullptr; anchorId.clear();
            logEvent("workspace.native-arrangement.owner-destroyed");
        });
        if (!scoped && cells.contains(nativeLeaf())) error=lease.retainNativeRemainder(cells.value(nativeLeaf()));
        if (!error.isEmpty()) { restoreOwned(false); return error; }
        if (!scoped && !cells.contains(nativeLeaf())) lease.preserveNativeVisibility();
        for (auto it=panels.begin();it!=panels.end();++it) {
            auto session=it.value(); error=session->attachToGroup(cells.value(it.key()),true);
            if (!error.isEmpty()) { restoreOwned(false); return error; }
            sessions[it.key()]=session;installHeader(it.key(),session);
            const auto id=it.key(); session->releaseGroup=[this,id] { detach(id,true); };
        }
        for (auto session : sessions) if (session) session->finishGroupTransfer();
        syncCellVisibility();
        logEvent("workspace.native-arrangement.create",{{"panels",QJsonArray::fromStringList(names)}});
        return {};
    }
    bool detach(const QString &id,bool deferHostShow = false) {
        auto session=sessions.value(id); auto cell=cells.value(id);
        if (changing || !active() || !session || !cell) return false;
        QScopedValueRollback<bool> guard(changing,true);
        UiWorkTiming timing("workspace.native-arrangement.detach",host->window());
        const auto next=removeGroupLeaf(snapshot(),id);
        removeHeader(id);session->releaseGroup={}; session->detachFromGroup(deferHostShow); sessions.remove(id); cells.remove(id);
        cell->hide(); cell->deleteLater();
        const bool result=sessions.isEmpty() ? restoreOwned(false) : rebuild(next);
        logEvent("workspace.native-arrangement.detach",{{"panel",id},{"success",result}});
        return result;
    }
    QString add(const QHash<QString,ManagedPanelSession *> &panels,const QJsonObject &addition,
        const QString &target,Qt::Orientation direction,bool before=false) {
        QStringList added,existing; const auto previous=snapshot();
        if (changing || !active() || panels.isEmpty() || sessions.size()+panels.size()>31 ||
            !groupTreeNames(addition,added) || added.size()!=panels.size() ||
            !groupTreeNames(previous,existing) || !groupTreeHasTarget(previous,target)) return QStringLiteral("Invalid native dock target.");
        QSet<ManagedPanelSession *> unique;
        for (auto current : sessions) if (current) unique.insert(current);
        for (auto it=panels.begin();it!=panels.end();++it) {
            auto session=it.value();
            if (!added.contains(it.key()) || existing.contains(it.key()) || !session || !session->active() || session->grouped() ||
                session->pageWidget()!=pageOwner || session->nativeOwnerWindow()!=host->window() || unique.contains(session))
                return QStringLiteral("Choose available panels from this native page.");
            unique.insert(session);
        }
        QScopedValueRollback<bool> guard(changing,true);
        UiWorkTiming timing("workspace.native-arrangement.add",host->window());
        for (auto it=panels.begin();it!=panels.end();++it) {
            auto cell=new QWidget(host); cell->setObjectName("unboundGroupCell_"+it.key()); cell->setProperty("groupPanel",it.key());
            cell->setWindowTitle(it.value()->panelTitle()); cells[it.key()]=cell;
        }
        QString error;
        if (!rebuild(insertGroupTree(previous,addition,target,direction,before))) error=QStringLiteral("Could not render the docked panels.");
        if (error.isEmpty()) for (auto it=panels.begin();it!=panels.end();++it) {
            error=it.value()->attachToGroup(cells.value(it.key()),true);
            if (!error.isEmpty()) break;
            sessions[it.key()]=it.value();installHeader(it.key(),it.value());
        }
        if (!error.isEmpty()) {
            for (const auto &id : added) {
                removeHeader(id);
                if (auto session=sessions.take(id)) session->detachFromGroup();
                if (auto cell=cells.take(id)) { cell->hide(); cell->deleteLater(); }
            }
            const bool restored=rebuild(previous);
            logEvent("workspace.native-arrangement.add-rollback",{{"restored",restored},{"error",error}});
            return error;
        }
        for (const auto &id : added) {
            auto session=sessions.value(id);
            session->releaseGroup=[this,id] { detach(id,true); }; session->finishGroupTransfer();
        }
        syncCellVisibility();
        logEvent("workspace.native-arrangement.add",{{"panels",QJsonArray::fromStringList(added)},{"target",target}});
        return {};
    }
    bool restore(bool dockPanels) {
        if (changing) return false;
        QScopedValueRollback<bool> guard(changing,true);
        return restoreOwned(dockPanels);
    }
private:
    bool restoreOwned(bool dockPanels) {
        UiWorkTiming timing("workspace.native-arrangement.restore",host ? host->window() : nullptr);
        if(active()&&prepareRestore&&!prepareRestore().isEmpty())return false;
        // Original slots must be back before sessions dock into them. If native
        // ownership changed, retain the arrangement rather than deleting content.
        if (!lease.restore()) return false;
        for(const auto &id:headers.keys())removeHeader(id);
        bool result=true;
        auto current=sessions.values();
        for (auto session : current) if (session) session->releaseGroup={};
        if (position.active()) {
            for (auto session : current) if (session) session->detachFromGroup(dockPanels);
            scopedLease.release();
            if (!anchorSession || !anchorSession->adoptOriginalPosition(position)) return false;
            anchorSession->releaseOriginalPosition={};
            if (dockPanels && !current.contains(anchorSession)) current.append(anchorSession);
            anchorSession=nullptr; anchorId.clear();
        }
        sessions.clear();
        for (auto session : current) if (session) {
            session->releaseGroup={}; session->detachFromGroup(dockPanels);
            if (dockPanels && session->active()) {
                const auto error=session->dock(); result &= error.isEmpty();
                if (!error.isEmpty()) logEvent("workspace.native-arrangement.dock-failed",{{"error",error}});
            }
        }
        retire(); return result;
    }
    bool rebuild(const QJsonObject &tree) {
        for(auto header:headers)if(header)header->park();
        PanelLayoutCallbacks callbacks;
        callbacks.changed=[this]{if(!changing){presentHeaders();if(restored)restored();}};
        callbacks.tearOff=[this](QWidget *leaf,QPoint pointer) {
            for (auto it=cells.begin();it!=cells.end();++it) if (it.value()==leaf && sessions.value(it.key())) {
                const auto id=it.key();
                logEvent("workspace.native-tab.tear-off",{{"panel",id},{"x",pointer.x()},{"y",pointer.y()}});
                if (tearOff) tearOff(id,pointer);
                return;
            }
            const auto tree=panelLayoutSnapshot(leaf); QStringList names;
            if (groupTreeNames(tree,names) && names.size()>1 && !names.contains(nativeLeaf()) && tearOffSection) {
                tearOffSection(tree,QRect(leaf->mapToGlobal(QPoint()),leaf->size()),pointer); return;
            }
            logEvent("workspace.native-tab.tear-off-rejected",{{"reason","native-remainder-or-invalid-section"}});
        };
        callbacks.leaf=[this](const QString &id) { return cells.value(id).data(); };
        callbacks.title=[this](const QString &id) {
            auto session=sessions.value(id);
            const auto title=session?session->panelTitle():cells.value(id)?cells.value(id)->windowTitle():QString();
            return !title.isEmpty()?title:id==nativeLeaf()?QStringLiteral("Native workspace"):QStringLiteral("Panel");
        };
        // Validate before moving any live cells out of their current surface.
        QSet<QString> ids; QList<QPointer<QWidget>> leaves;
        if (!validatePanelLayoutSurface(tree,callbacks,ids,leaves)) return false;
        for (auto cell : cells) if (cell) cell->setParent(host);
        visibility.reset();
        if (root) {
            host->layout()->removeWidget(root);
            // A one-panel scoped arrangement renders the retained cell itself.
            // Retire only generated split/tab containers, never that live leaf.
            if (!cells.values().contains(root)) { root->hide(); root->deleteLater(); }
        }
        root=buildPanelLayoutSurface(tree,host,this,callbacks);
        if (!root) return false;
        host->layout()->addWidget(root); root->show();
        syncCellVisibility();
        host->layout()->activate();
        restorePanelLayoutProportions(root);
        QList<QWidget *> targets; for (auto cell : cells) if (cell) targets.append(cell);
        if (!registerNativeDockSurfaces(host->window(),host,targets)) return false;
        return true;
    }
    void syncCellVisibility() {
        applyPanelLayoutVisibility(root,visibility,[this](const QString &id) {
            auto session=sessions.value(id); return !session || !session->hiddenByUser();
        });
        presentHeaders();
    }
    void installHeader(const QString &id,ManagedPanelSession *session) {
        auto cell=cells.value(id);if(!cell || !session || headers.contains(id))return;
        auto layout=qobject_cast<QBoxLayout *>(cell->layout());if(!layout)return;
        QPointer<ManagedPanelSession> alive=session;
        auto header=new GroupPanelHeader(cell,session->panelTitle(),[alive]{return alive?alive->interfaceScale():1.;},
            [alive,this](double factor){if(alive)alive->setInterfaceScale(factor);if(restored)restored();},
            [this,id]{if(detachRequested)detachRequested(id);},
            [this,id]{if(setPanelVisible(id,false)&&restored)restored();},{});
        header->label->setProperty("groupPanel",id);header->label->setProperty("openGatePanelTitleGesture",true);
        new PanelTitleGesture(header->label,header,[this,id](QPoint pointer){QTimer::singleShot(0,this,[this,id,pointer]{if(tearOff)tearOff(id,pointer);});});
        header->button->drag=[this,id](QPoint pointer){QTimer::singleShot(0,this,[this,id,pointer]{if(tearOff)tearOff(id,pointer);});};
        layout->insertWidget(0,header->bar);headers[id]=header;
        header->borrowNativeHeader(cell,id);
        setNativeGroupSurfaceClipping(cell);
    }
    void removeHeader(const QString &id) {
        auto header=headers.take(id);if(!header)return;
        header->releaseNativeHeader();header->park();auto bar=header->bar;delete bar.data();header->deleteLater();
    }
    void presentHeaders() {
        // Corner controls must leave the old tab before another header takes it.
        for(auto header:headers)if(header)header->park();
        for(auto it=headers.begin();it!=headers.end();++it)if(it.value()) {
            it.value()->presentFor(cells.value(it.key()));
        }
    }
    void retire() {
        for(const auto &id:headers.keys())removeHeader(id);
        unregisterNativeDockSurfaces(host);
        disconnect(ownerDestroyed); ownerDestroyed={};
        if (host) { host->hide(); host->deleteLater(); }
        host=nullptr; root=nullptr; pageOwner=nullptr; cells.clear();
    }
    NativeRootLayout lease;
    NativePanelPosition position;
    NativeGroupSlot scopedLease;
    QPointer<ManagedPanelSession> anchorSession;
    QString anchorId;
    QMetaObject::Connection ownerDestroyed;
    GroupVisibility visibility;
    QPointer<QWidget> host,root,pageOwner;
    QHash<QString,QPointer<QWidget>> cells;
    QHash<QString,QPointer<ManagedPanelSession>> sessions;
    QHash<QString,QPointer<GroupPanelHeader>> headers;
    bool changing=false;
};
}
