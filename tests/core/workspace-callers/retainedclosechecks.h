// SPDX-License-Identifier: MIT
#pragma once

// Uses the assembled first-open fixture and real coordinator entry points.
// No simulated picture is treated as native loading acceptance.
inline void checkRetainedClose(PanelControls &controls, QWidget &main, QStackedWidget &pages,
    QWidget *page, QWidget *otherPage, const std::function<void(bool,const char *)> &check)
{
    using namespace Unbound;
    auto binding=component<NativeWorkspaceBinding>(&controls);
    auto callers=component<WorkspacePanelCallers>(&controls);
    auto session=[&](const QString &title) -> ManagedPanelSession * {
        for(auto child:controls.children()) if(auto value=dynamic_cast<ManagedPanelSession *>(child))
            if(value->panelTitle()==title)return value;
        return nullptr;
    };
    auto button=[&](const QString &id) {
        auto value=controls.findChild<QPushButton *>(id);check(value!=nullptr,"fixture finds production action");
        if(value)value->click();settle(400);
    };
    auto recall=[&](const QString &id) {
        QMenu menu;callers->populateFlat(&menu);
        auto action=menu.findChild<QAction *>("openGatePanelToggle_"+id);
        check(action&&action->isEnabled()&&!action->isChecked(),"Panels identifies the closed destination as hidden");
        if(action)action->trigger();settle(400);
    };
    auto group=[&]() -> PanelGroup * {
        for(auto child:controls.children())if(auto value=dynamic_cast<PanelGroup *>(child))if(value->active())return value;
        return nullptr;
    };
    if(!binding||!callers){check(false,"assembled close fixture has binding and callers");return;}
    if(QCoreApplication::arguments().contains("--dock-baseline")) {
        button("undock_widgetEditInspector");button("dock_widgetEditInspector");
        auto inspector=session("Inspector");
        check(!inspector||!inspector->active(),"baseline explicit Dock without any Close releases the full-layout Inspector lease");
        std::fprintf(stderr,"Baseline Dock result: %s\n",qPrintable(component<PanelManagerView>(&controls)->status->text()));
        return;
    }
    if(QCoreApplication::arguments().contains("--restart-check")) {
        auto retained=group();auto source=session("Source Monitor");auto inspector=session("Inspector");
        check(retained&&source&&inspector,"restart restores retained group and standalone session");
        if(!retained||!source||!inspector)return;
        check(retained->windowHidden()&&!retained->window()->isVisible()&&inspector->hiddenByUser()&&!inspector->window()->isVisible(),
            "fresh process preserves closed group and standalone visibility");
        auto before=retained->snapshot();auto host=retained->window();
        recall("widgetEditSourcePlayer");
        check(retained->window()==host&&host->isVisible()&&retained->selected(source)&&host->property("unboundPinned").toBool(),
            "fresh process recall selects Source in the retained pinned group");
        check(retained->snapshot()["tree"]==before["tree"]&&retained->snapshot()["panels"]==before["panels"],
            "fresh process recall preserves group tree and membership");
        recall("widgetEditInspector");
        check(inspector->window()->isVisible()&&inspector->pinned(),"fresh process standalone recall preserves pinning");
        return;
    }

    button("undock_widgetEditSourcePlayer");auto source=session("Source Monitor");
    if(!source||!source->floating()){check(false,"Source detaches through production controls");return;}
    QPointer<QWidget> host=source->window();source->setPinned(true);settle(100);const auto frame=host->geometry();
    check(host->close(),"standalone Close is accepted");settle(400);
    check(source->active()&&source->hiddenByUser()&&host&&source->window()==host&&!host->isVisible(),
        "Close hides the existing host instead of docking or destroying it");
    if(!source->active()||!host)return;
    pages.setCurrentWidget(otherPage);settle();pages.setCurrentWidget(page);settle();
    check(source->window()==host&&!host->isVisible(),"page return does not reopen a closed standalone panel");
    recall("widgetEditSourcePlayer");

    check(source->window()==host&&host->geometry()==frame&&source->pinned(),"Panels recall retains standalone identity geometry and pinning");
    button("expand_widgetEditSourcePlayer");check(host->property("unboundExpanded").toBool(),"standalone expands before Close");
    host->close();settle(400);
    check(source->active()&&source->window()==host&&host->isVisible()&&host->geometry()==frame&&!host->property("unboundExpanded").toBool(),
        "Close on temporary standalone expansion restores size without hiding or docking");

    button("undock_widgetEditTimelinePlayer");
    button("group_0");auto retained=group();
    check(retained&&source->grouped(),"production grouping creates retained Source and Program group");
    if(!retained||!source->grouped())return;
    auto program=session("Program Monitor");
    const auto nested=splitGroupTree(splitGroupTree({{"panel","widgetEditSourcePlayer"}},{{"panel","widgetEditTimelinePlayer"}},Qt::Orientation(0)),
        {{"panel","widgetEditInspector"}},Qt::Horizontal);
    button("undock_widgetEditInspector");auto inspector=session("Inspector");
    check(inspector&&retained->add(inspector,Qt::Horizontal).isEmpty(),"group accepts Inspector for nested tab recall");
    if(!inspector||!program)return;
    auto state=retained->snapshot();state["tree"]=nested;state["pinned"]=true;
    check(retained->restore(state),"fixture retains nested tabs and pins whole group");settle(200);
    inspector->setHiddenByUser(true,false);retained->syncVisibility();settle(100);
    QPointer<QWidget> groupHost=retained->window();const auto groupFrame=groupHost->geometry();const auto tree=retained->snapshot().value("tree");
    groupHost->close();settle(400);
    check(retained->active()&&retained->windowHidden()&&groupHost&&!groupHost->isVisible(),"group Close hides without dissolution");
    if(!retained->active()||!groupHost)return;
    check(source->grouped()&&program->grouped()&&!source->hiddenByUser()&&inspector->hiddenByUser(),"whole-group Close preserves individual hidden choices");
    pages.setCurrentWidget(otherPage);settle();pages.setCurrentWidget(page);settle();
    check(!groupHost->isVisible(),"page return does not reopen a closed group");
    recall("widgetEditSourcePlayer");

    check(retained->window()==groupHost&&groupHost->geometry()==groupFrame&&retained->snapshot()["tree"]==tree&&retained->selected(source),
        "nested recall preserves group frame tree and selected Source path");
    check(inspector->hiddenByUser()&&groupHost->property("unboundPinned").toBool(),"recall retains hidden sibling and group pinning");
    button("expand_widgetEditSourcePlayer");auto expanded=source->expandedWindow();
    check(expanded!=nullptr,"grouped Source expands independently");if(expanded)expanded->close();settle(400);
    check(!source->expandedWindow()&&retained->window()==groupHost&&!retained->windowHidden(),"Close on grouped expansion restores retained group");

    retained->detachMember(inspector);inspector->setPinned(true);inspector->setHiddenByUser(false,false);settle(100);
    inspector->window()->close();groupHost->close();settle(700);
    WorkspaceSettings saved;const auto checkpoint=saved.pageView("edit");
    check(checkpoint["panels"].toObject()["widgetEditInspector"].toObject()["hidden"].toBool()&&
        checkpoint["groups"].toArray().size()==1&&checkpoint["groups"].toArray().first().toObject()["hidden"].toBool(),
        "unsaved workspace persists closed standalone and group states");
    QStringList restartArguments{"--restart-check"};
#ifdef OPENGATE_TEST_PRODUCTION_BOOTSTRAP
    restartArguments.append(QCoreApplication::arguments().last());
#endif
    QProcess restart;restart.start(QCoreApplication::applicationFilePath(),restartArguments);
    const bool finished=restart.waitForFinished(20000);
    if(!finished||restart.exitCode()!=0)std::fprintf(stderr,"Restart output: %s %s\n",restart.readAllStandardOutput().constData(),restart.readAllStandardError().constData());
    check(finished&&restart.exitStatus()==QProcess::NormalExit&&restart.exitCode()==0,"fresh-process close and recall acceptance passes");

    // Restore a named snapshot through the same preset transaction used by the UI.
    ArrangementStore store;QString id;check(store.create("Closed windows",checkpoint,id).isEmpty(),"named workspace stores closed arrangement");
    auto restoration=component<WorkspaceRestoration>(&controls);
    restoration->preset(store.find(id),true);settle(2000);
    retained=group();inspector=session("Inspector");
    check(retained&&retained->windowHidden()&&inspector&&inspector->hiddenByUser(),"named preset restoration preserves Close state");
    auto nativeAction=main.findChild<QAction *>("unboundRestoreNativeWorkspace");
    check(nativeAction!=nullptr,"native return remains accessible");if(nativeAction)nativeAction->trigger();settle(700);
    check(!binding->active()&&!inspector->active(),"native return releases closed hosts and restores native ownership");
    button("undock_widgetEditInspector");inspector=session("Inspector");
    if(!inspector){check(false,"Inspector can float again after native return");return;}
    inspector->window()->close();settle(300);button("dock_widgetEditInspector");
    check(!inspector->active(),"explicit Dock restores a closed panel to its original native slot");
    restoration->preset(store.find(id),true);settle(2000);
    // Shutdown has its own direct release path; it must not use Close to dock.
    QMetaObject::invokeMethod(qApp,"aboutToQuit",Qt::DirectConnection);settle(100);
    check(!binding->active(),"shutdown releases the native workspace");
    for(auto child:controls.children())if(auto value=dynamic_cast<ManagedPanelSession *>(child))check(!value->active(),"shutdown docks every retained session");
    WorkspaceSettings afterQuit;check(afterQuit.pageView("edit")["groups"]==checkpoint["groups"],"shutdown preserves the closed group checkpoint before releasing ownership");
    check(main.isVisible(),"fixture never closes the native main window");
}
