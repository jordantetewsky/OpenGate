// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "workspacesettings.h"
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);QTemporaryDir dir;qputenv("UNBOUND_WORKSPACE_FILE",(dir.path()+"/Workspace.json").toUtf8());
 int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
 QJsonObject edit{{"page","edit"},{"panels",QJsonObject{{"timeline",QJsonObject{{"floating",true},{"x",42}}}}},{"selected_arrangement","edit-id"}};
 QJsonObject initialNative{{"page","edit"},{"panels",QJsonObject{}}};
 QJsonObject automatic=initialNative;
 check(Unbound::PageWorkspaceState::record(automatic,"edit",initialNative).isEmpty(),"initial Native mode records");
 check(Unbound::PageWorkspaceState::record(automatic,"edit",edit).isEmpty(),"first floating panel automatically records managed mode");
 const auto automaticView=Unbound::PageWorkspaceState::view(automatic,"edit");
 check(automaticView["layout_mode"]=="managed" && automaticView["panels"]==edit["panels"] && automaticView["selected_arrangement"]==edit["selected_arrangement"],"automatic flow preserves captured geometry and preset identity without loading a starter");
 QJsonObject hiddenNative{{"page","edit"},{"panels",QJsonObject{{"source",QJsonObject{{"native_hidden",true}}}}}};
 check(Unbound::PageWorkspaceState::mode(hiddenNative)=="managed","unsaved native visibility change is a modified layout");
 QJsonObject visibilityRoot=initialNative;
 check(Unbound::PageWorkspaceState::record(visibilityRoot,"edit",hiddenNative).isEmpty(),"native visibility checkpoint records");
 check(Unbound::PageWorkspaceState::view(visibilityRoot,"edit")["panels"]==hiddenNative["panels"],"native visibility survives page ledger round trip");
 QJsonObject root=edit;
 check(Unbound::PageWorkspaceState::record(root,"edit",edit).isEmpty(),"migrate legacy Edit without losing modified geometry");
 auto fusion=QJsonObject{{"page","fusion"},{"groups",QJsonArray{QJsonObject{{"custom",true}}}},{"selected_arrangement","fusion-id"}};
 check(Unbound::PageWorkspaceState::record(root,"fusion",fusion).isEmpty(),"record independent Fusion view");
 const auto before=root;check(!Unbound::PageWorkspaceState::record(root,"fusion",edit).isEmpty() && root==before,"cross-page record fails atomically");
 check(root["selected_arrangement"]=="edit-id" && Unbound::PageWorkspaceState::view(root,"fusion")["selected_arrangement"]=="fusion-id","Fusion cannot replace Edit selection");
 QJsonObject nativeEdit{{"page","edit"},{"panels",QJsonObject{}},{"selected_arrangement","edit-id"}};
 check(Unbound::PageWorkspaceState::record(root,"edit",nativeEdit).isEmpty(),"explicit Native Edit transition records Native mode");
 check(Unbound::PageWorkspaceState::view(root,"edit")["layout_mode"]=="native" && Unbound::PageWorkspaceState::view(root,"fusion")["layout_mode"]=="managed","Native Edit and custom Fusion modes remain independent");
 check(Unbound::PageWorkspaceState::managedView(root,"edit")["panels"]==edit["panels"],"Native return retains last modified custom layout independently of preset");
 Unbound::WorkspaceSettings settings;settings.data=root;check(settings.save(),"save page state atomically");
 Unbound::WorkspaceSettings reopened;
 check(reopened.pageView("fusion")==Unbound::PageWorkspaceState::view(root,"fusion") && reopened.pageView("edit")==Unbound::PageWorkspaceState::view(root,"edit"),"restart preserves both page modes and layouts");
 check(!reopened.pageView("fusion").contains("page_workspaces"),"page snapshot cannot recursively capture other pages");
 // Simulate process exit during a native-return transaction: legacy fields can
 // be intermediate, while the page checkpoint remains the last complete view.
 const auto stablePages=root["page_workspaces"].toObject();
 settings.data=QJsonObject{{"page","edit"},{"intermediate",true},{"page_workspaces",stablePages}};
 check(settings.save(),"write interrupted-transition fixture");
 Unbound::WorkspaceSettings recovered;
 check(!recovered.data.contains("intermediate") && recovered.pageView("fusion")==reopened.pageView("fusion"),"restart ignores intermediate legacy projection and preserves other page");
 auto future=root;auto pages=future["page_workspaces"].toObject();pages["fusion"]=QJsonObject{{"version",99}};future["page_workspaces"]=pages;const auto original=future;
 check(!Unbound::PageWorkspaceState::record(future,"fusion",fusion).isEmpty() && future==original,"unknown page state version preserved");
 for(const auto &invalid:{QByteArray("broken-json"),QByteArray("{\"version\":99,\"do_not_lose\":true}")}) {
  QFile damaged(dir.path()+"/Workspace.json");check(damaged.open(QIODevice::WriteOnly),"open damaged fixture");damaged.write(invalid);damaged.close();
  Unbound::WorkspaceSettings protectedState;protectedState.data={{"version",1}};
  check(!protectedState.error().isEmpty() && !protectedState.save(),"invalid/unknown workspace blocks automatic overwrite");
  damaged.open(QIODevice::ReadOnly);check(damaged.readAll()==invalid,"original invalid bytes remain recoverable");damaged.close();
 }
 std::printf("page workspace state failures=%d\n",failures);return failures?1:0;
}
