// SPDX-License-Identifier: MIT
#pragma once
#include "workspacesnapshot.h"
#include "panelexpansioncontroller.h"
#include "nativeworkspacebinding.h"
#include "nativesurfacedoors.h"
#include "footerpanel.h"
#include "pinnedworkspaceheader.h"
#include "uiworktiming.h"

namespace Unbound {
// Captures the existing owners and commits the established settings format.
// Sources are borrowed for one synchronous call, never retained as another model.
// This component does not move widgets, restore layouts, wire UI or schedule work.
class WorkspacePersistence {
public:
    explicit WorkspacePersistence(WorkspaceSettings &settings) : settings(settings) {}
    struct Panel {
        QString name;
        bool desired;
        ManagedPanelSession *session;
        QJsonObject *saved;
    };
    struct Sources {
        QList<Panel> panels;
        QList<PanelGroup *> groups;
        QJsonArray pendingGroups;
        NativeWorkspaceBinding *nativeWorkspace=nullptr;
        NativeSurfaceDoors *nativeDoors=nullptr;
        PanelExpansionController *expansion=nullptr;
        FooterPanel *footer=nullptr;
        PinnedWorkspaceHeader *header=nullptr;
        QWidget *main=nullptr;
        QJsonObject *mainPlacement=nullptr;
    };
    struct Request {
        bool quitting=false, restorationBlocked=false, restoringNative=false, retainSelection=false;
        QString mode;
    };
    struct Result {
        bool performed=false, success=false;
        double snapshotMs=0, persistMs=0;
    };

    // Expansion needs this in-memory checkpoint without a disk write or UI work.
    QJsonObject capturePanels(const Sources &source) const {
        QJsonObject saved;
        for (int i=0;i<source.panels.size();++i) {
            const auto &panel=source.panels[i];
            auto session=panel.session;
            auto value=WorkspaceSnapshot::panel(*panel.saved,panel.desired,session->active(),
                session->active()?session->floatingGeometry():QRect(),session->hiddenByUser(),session->pinned());
            if (!session->active() && source.nativeDoors->contains(panel.name)) {
                if (source.nativeDoors->modified(panel.name)) value["native_hidden"]=source.nativeDoors->hidden(panel.name);
                else value.remove("native_hidden");
            }
            if (session->active() && !session->grouped()) value["maximized"]=session->windowMaximized();
            value["interface_scale"]=session->interfaceScale();
            if (panel.name=="unboundWorkspaceToolbar" && session->active() && !session->grouped())
                value["toolbar_float_pinned"]=session->pinned();
            value=source.expansion->panelSnapshot(i,value);
            *panel.saved=value;
            saved[panel.name]=value;
        }
        return saved;
    }

    Result save(const Request &request,const Sources &source,WorkspaceSnapshot options) {
        if (request.quitting) return {};
        if (request.restorationBlocked) {
            logEvent("workspace.save.deferred",{{"reason","workspace-restore-transaction"}});
            return {};
        }
        if (!request.restoringNative && (!source.pendingGroups.isEmpty() ||
            (source.nativeWorkspace && !source.nativeWorkspace->requiredPanels().isEmpty()))) {
            logEvent("workspace.save.deferred",{{"reason","workspace-assembly-pending"}});
            return {};
        }
        // Preserve the previous complete page checkpoint until assembly settles.
        UiWorkTiming timing("workspace.capture-persist",source.main);
        QElapsedTimer phases;phases.start();
        const auto captured=capture(source,std::move(options),request);
        const auto previousMode=PageWorkspaceState::mode(settings.pageView("edit"));
        QString pageError;
        if (request.restoringNative) {
            const auto pages=settings.data["page_workspaces"].toObject();
            settings.data=captured;
            if (!pages.isEmpty()) settings.data["page_workspaces"]=pages;
        } else pageError=settings.recordPage("edit",captured);
        Result result;result.performed=true;
        result.snapshotMs=phases.nsecsElapsed()/1000000.;phases.restart();
        result.success=pageError.isEmpty() && settings.save();
        result.persistMs=phases.nsecsElapsed()/1000000.;
        if (!pageError.isEmpty() || !settings.error().isEmpty())
            logEvent("workspace.page-state.failed",{{"page","edit"},{"error",pageError.isEmpty()?settings.error():pageError}});
        if (result.success && !request.restoringNative && previousMode!=PageWorkspaceState::mode(captured))
            logEvent("workspace.mode.changed",{{"page","edit"},{"before",previousMode},
                {"mode",PageWorkspaceState::mode(captured)},{"preset_loaded",false},
                {"layout_policy","retain-existing-native-and-managed-surfaces"}});
        logEvent("workspace.save",{{"success",result.success},{"page","edit"},
            {"layout_mode",PageWorkspaceState::mode(captured)},{"selected",captured["selected_arrangement"]},
            {"page_state_checkpoint",!request.restoringNative}});
        return result;
    }

private:
    QJsonObject capture(const Sources &source,WorkspaceSnapshot snapshot,const Request &request) const {
        snapshot.panels=capturePanels(source);
        snapshot.groups=source.pendingGroups;
        for (auto group:source.groups) if (group->active()) snapshot.groups.append(source.expansion->groupSnapshot(group));
        snapshot.nativeArrangement=source.expansion->nativeSnapshot(source.nativeWorkspace?source.nativeWorkspace->saved():QJsonObject());
        snapshot.footer=source.footer?source.footer->snapshot():QJsonObject();
        snapshot.header=source.header?source.header->snapshot():QJsonObject();
        if (request.retainSelection) snapshot.selected=settings.pageView("edit")["selected_arrangement"].toString();
        auto captured=snapshot.json();captured["page"]="edit";captured["layout_mode"]=request.mode;
        if (source.main) {
            auto placement=WorkspaceSettings::displayRectangle(
                source.main->isMaximized()?source.main->normalGeometry():source.main->geometry(),*source.mainPlacement);
            *source.mainPlacement=placement;placement["maximized"]=source.main->isMaximized();
            captured["main_window"]=placement;
        }
        return captured;
    }
    WorkspaceSettings &settings;
};
}
