// SPDX-License-Identifier: MIT
#pragma once
#include <QElapsedTimer>
#include "panelmanagerview.h"
#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>
#include "panelsession.h"
#include "workspacesettings.h"
#include "workspacepersistence.h"
#include "workspacerestoration.h"
#include "windowhierarchy.h"
#include "arrangementstore.h"
#include "panelmenus.h"
#include "nativepaneldecoration.h"
#include "timelinecontrol.h"
#include "nativeworkspacesizing.h"
#include "nativepaneltitledrags.h"
#include "nativeworkspacepresentation.h"
#include "nativepaneldrag.h"
#include "nativeworkspacebinding.h"
#include "panelgroup.h"
#include "interactiondiagnostics.h"
#include "controlscroll.h"
#include "viewerpreparation.h"
#include "surfacereadiness.h"
#include "footerpanel.h"
#include "panelexpansioncontroller.h"
#include "workspaceviewport.h"
#include "panelhovertarget.h"
#include "workspacecallers.h"
#include "toolbars.h"
#include "toolbardoors.h"
#include "nativepanelvisibility.h"
#include "nativesurfacedoors.h"
#include "attachedtoolbarreveal.h"
#include "pinnedworkspaceheader.h"
#include "workspaceheadermigration.h"
#include "viewermodecommand.h"
#include "arrangementcontrols.h"
#include "workspacearrangements.h"
#include "panelcallermenu.h"
#include "panelcommandcatalog.h"
#include "panelcommandmenu.h"
#include "nativeeditcommands.h"
#include "nativepaneltoggles.h"
#include "pagecontext.h"
#include "editpagelifecycle.h"
#include "workspaceheaders.h"
#include "nativepanelnavigation.h"

class PanelControls : public QWidget
{
    struct Panel {
        QString label, name, toggle;
        bool contents = false, desired = false, pending = false;
        unsigned requestVersion = 0;
        Unbound::ManagedPanelSession *session = nullptr;
        QPushButton *undock = nullptr, *dock = nullptr, *show = nullptr, *hide = nullptr, *expand = nullptr, *center = nullptr;
        QJsonObject saved = {};
    };
public:
    std::function<bool(int,bool)> audioExperiment;
    PanelControls() : QWidget(nullptr, Qt::Window)
    {
        setObjectName(QStringLiteral("unboundPanelTest"));
        setWindowTitle(QStringLiteral("OpenGate Panel Manager"));
        setAttribute(Qt::WA_QuitOnClose, false);
        Unbound::logEvent(QStringLiteral("controls.create"));
        if(!settings.hasSavedWorkspace() && settings.error().isEmpty()) {
            settings.data=Unbound::resolveLayoutEntry("edit")["workspace"].toObject();
            settings.data["selected_arrangement"]=Unbound::WorkspacePresetPolicy::resolveLayoutId("edit");
        }
        layoutMode=Unbound::PageWorkspaceState::mode(settings.data);pendingMainPlacement=settings.data["main_window"].toObject();mainPlacementRecord=pendingMainPlacement;
        managerView=new Unbound::PanelManagerView(this,settings.data);
        status=managerView->status; presetsUi=managerView->presets;
        optionsScroll=managerView->scroll; pin=managerView->pin; constrain=managerView->constrain;
        snap=managerView->snap; background=managerView->background;
        // Legacy presets remain recoverable, but their former manual background
        // policy must not hide the new primary workspace behind its panels.
        {const QSignalBlocker block(background);background->setChecked(false);}
        hierarchyOption=managerView->hierarchy; style=managerView->style;
        presets = presetsUi->selection();
        workspaceRestore=new Unbound::WorkspaceRestoration(this);
        workspaceRestore->running=[this] { return !quitting; };
        workspaceRestore->checkpoint=[this] { save(true);return settings.pageView("edit"); };
        workspaceRestore->report=[this](QString message) { status->setText(message); };
        workspaceRestore->validate=[this](const QJsonObject &value) {
            return nativeWorkspace->validate(value["native_arrangement"].toObject(),value["groups"].toArray());
        };
        workspaceRestore->dock=[this] {
            ++bulkVersion; ++groupVersion; restoreExpanded();
            const QScopedValueRollback<bool> transaction(restoringNative,true);
            const auto error=dockAll(false);if(!error.isEmpty())return error;
            for (const auto &panel:panels) if (panel.session->active()) return panel.name;
            return QString();
        };
        workspaceRestore->apply=[this](const QJsonObject &value) { return applyWorkspace(value); };
        workspaceRestore->resume=[this] { restore(); save(); };
        workspaceRestore->committed=[this] { syncNativeToggles(layoutMode=="native");save(); };
        workspaceRestore->groupsPending=[this] { return !pendingGroups.isEmpty() || !nativeWorkspace->requiredPanels().isEmpty(); };
        workspaceRestore->unavailable=[this] {
            QStringList missing;
            for (const auto &panel:panels) if (panel.desired && !panel.session->active()) missing.append(panel.label);
            return missing;
        };
        presetsUi->workspace = [this] { return arrangement(); };
        presetsUi->changed = [this] { save(); };
        presetsUi->restoreRequested = [this] { restoreArrangement(); };
        presetsUi->report = [this](QString message) { status->setText(message); };
        connect(background, &QCheckBox::toggled, this, [this](bool hidden) {
            action(QStringLiteral("background-visibility"));
            if (!applyBackgroundVisibility(hidden)) { const QSignalBlocker blocker(background); background->setChecked(!hidden); }
            save();
        });
        connect(snap, &QCheckBox::toggled, this, [this](bool enabled) {
            action(QStringLiteral("snap-windows"));
            for (auto &panel : panels) panel.session->setSnapping(enabled);
            for (auto group : groups) group->setSnapping(enabled);
            save();
        });
        expansionController=new Unbound::PanelExpansionController(this);
        expansionController->running=[this] { return !quitting; };
        expansionController->update=[this](int i) { updateRow(i); };
        expansionController->save=[this] { if (!quitting) saveTimer.start(); };
        expansionController->report=[this](QString error) { status->setText(error); };
        hierarchy = new Unbound::WindowHierarchy(this);
        groupReserve = new Unbound::GroupHostReserve(this);
        nativeSizing = new Unbound::NativeWorkspaceSizing(this);
        nativeSizing->blocked = [this] { return quitting || expansionController->index() >= 0; };
        hierarchy->setEnabled(hierarchyOption->isChecked());
        connect(hierarchyOption, &QCheckBox::toggled, this, [this](bool enabled) {
            action("panel-hierarchy"); hierarchy->setEnabled(enabled); save();
        });
        connect(style, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int value) {
            action(QStringLiteral("window-style-%1").arg(value));
            for (auto &panel : panels) panel.session->setChrome(static_cast<Unbound::WindowChrome>(value));
            for (auto group : groups) group->setChrome(static_cast<Unbound::WindowChrome>(value));
            rescue(); save();
        });
        setWindowFlag(Qt::WindowStaysOnTopHint, pin->isChecked());
        connect(pin, &QCheckBox::toggled, this, [this](bool value) {
            action("pin-controls"); const bool visible = isVisible();
            setWindowFlag(Qt::WindowStaysOnTopHint, value);
            if (visible) { show(); raise(); }
            save();
        });
        connect(constrain, &QCheckBox::toggled, this, [this](bool value) {
            action("constrain-windows");
            for (auto &panel : panels) panel.session->setConstrained(value);
            save();
        });
        for (int i=0;i<managerView->bulkButtons.size();++i) {
            auto button=managerView->bulkButtons[i];
            connect(button, &QPushButton::clicked, this, [this, i] {
                action(QStringLiteral("bulk-%1").arg(i));
                const unsigned command = ++bulkVersion;
                if (i == 0) {
                    if (nativeWorkspace && !nativeWorkspace->release()) { status->setText("Could not release the native arrangement safely."); return; }
                    for (int j = 0; j < panels.size(); ++j)
                        QTimer::singleShot(j * 80, this, [this, j, command] { if (command == bulkVersion) requestUndock(j); });
                    QTimer::singleShot(panels.size() * 80 + 200, this, [this, command] {
                        if (command != bulkVersion) return;
                        int count = 0; QStringList missing;
                        for (const auto &panel : panels) {
                            if (panel.session->active()) ++count;
                            else missing.append(panel.label);
                        }
                        status->setText(QStringLiteral("%1 of %2 panels floating.").arg(count).arg(panels.size()) +
                            (missing.isEmpty() ? QString() : QStringLiteral(" Not available: ") + missing.join(", ")));
                        Unbound::logEvent(QStringLiteral("bulk.undock.complete"), {{"floating", count}, {"total", panels.size()},
                            {"unavailable", missing.join(", ")}});
                    });
                } else if (i == 1) dockAll();
                else if (i == 5) quickExport();
                else for (int j=0;j<panels.size();++j) if (panels[j].session->active()) {
                    if (i == 2 || i == 3) showPanel(j,i==2);
                    else if (panels[j].session->floating()) panels[j].session->centerOnCurrentDisplay();
                }
                save();
            });
        }
        panels = {
            {"Inspector", "widgetEditInspector", "Inspector", false},
            {"Timeline", "frameConformTimelines", "", false},
            {"Source Monitor", "widgetEditSourcePlayer", "", false},
            {"Program Monitor", "widgetEditTimelinePlayer", "", false},
            {"Media Pool", "frameEditPageMediaPool", "MediaPool", true},
            {"Index", "frameEditPageEditIndex", "EditIndex", true},
            {"Effects", "frameEditPageEffectsLibrary", "EffectsLibrary", true},
            {"Metadata", "frameEditPageMetadata", "Metadata", true},
            {"Mixer", "frameEditAudioMeter", "Mixer", true},
            {"Sound Library", "frameEditPageSoundLibrary", "SoundLibrary", true},
            {"Keyframes", "frameEditPageKeyframeEditor", "KeyframeEditor", true},
            {"Timeline Toolbar", "frameEditToolbarTop", "", false},
            {"Workspace Toolbar", "unboundWorkspaceToolbar", "", false}
        };
        settings.data=Unbound::migrateWorkspaceHeader(settings.data);
        const QJsonObject savedPanels = settings.data["panels"].toObject();
        pendingGroups = settings.data["groups"].toArray();
        nativeMenus = new Unbound::PanelMenus(this);
        nativeWorkspace = new Unbound::NativeWorkspaceBinding(this);
        titleDrags=new Unbound::NativePanelTitleDrags(this);
        nativeWorkspace->sectionDetached=[this](Unbound::PanelGroup *group) { configureGroup(group); };
        nativeWorkspace->prepareMount=[this]() -> QString {return workspaceHeader && !workspaceHeader->release() ? QStringLiteral("Workspace header restoration needs recovery.") : QString();};
        nativeWorkspace->beforeMount = [this] {
            if (nativeSizing) nativeSizing->restore();
            const QSignalBlocker block(background); background->setChecked(false); applyBackgroundVisibility(false);
        };
        nativeWorkspace->changed = [this] {
            for (int i=0;i<panels.size();++i) updateRow(i);
            syncNativeToggles();
            nativeWorkspace->refreshNativeTargets(page());
            if (!quitting) {
                saveTimer.start();
                // restoreReady may finish in the binding's queued callback,
                // after the coordinator's current pass has already returned.
                if(workspaceRestore && workspaceRestore->active())restoreTimer.start(0);
            }
        };
        nativeWorkspace->failed = [this](const QString &error) {
            if(!workspaceRestore->failPreset(error))status->setText(error);
        };
        QStringList panelLabels; for (const auto &panel:panels) panelLabels.append(panel.label);
        managerView->addGroups(panelLabels);
        auto firstPanel=managerView->first,secondPanel=managerView->second;
        for (int i=0;i<managerView->groupButtons.size();++i) {
            auto button=managerView->groupButtons[i];
            connect(button, &QPushButton::clicked, this, [this, firstPanel, secondPanel, i] {
                action(QStringLiteral("group-%1").arg(i));
                if (i == 2) { releaseGroups(); save(); return; }
                const int a = firstPanel->currentIndex(), b = secondPanel->currentIndex();
                if (a == b) { status->setText(QStringLiteral("Choose two different panels.")); return; }
                restoreExpanded();
                requestUndock(a, true); requestUndock(b, true);
                const unsigned request = ++groupVersion;
                new Unbound::SurfaceReadiness(this,page(),
                    [this,request] { return !quitting && request==groupVersion; },
                    [this,a,b] { return panels[a].session->active() && panels[b].session->active() && !panels[a].pending && !panels[b].pending; },
                    [this, a, b, i, request](bool ready) {
                    if (quitting || request != groupVersion) return;
                    const auto error = combinePanels(a, b, i == 3 ? Qt::Orientation(0) : i == 0 ? Qt::Horizontal : Qt::Vertical);
                    Unbound::logEvent("group.prepare.result",{{"ready",ready},{"success",error.isEmpty()},{"error",error}});
                    status->setText(error.isEmpty() ? QStringLiteral("Panels joined. Drag the group grip; resize the shared divider.") : error);
                    save();
                });
            });
        }
        for (int i = 0; i < panels.size(); ++i) {
            auto &panel = panels[i];
            panel.saved = savedPanels[panel.name].toObject();
            panel.desired = panel.saved["floating"].toBool();
            panel.session = new Unbound::ManagedPanelSession(this);
            panel.session->beforeLayoutTransfer=[this] {return releaseWorkspaceBands();};
            panel.session->setInterfaceScale(panel.saved["interface_scale"].toDouble(1.0));
            nativeWorkspace->registerPanel(panel.name,panel.session,panel.label);
            panel.session->attachRequested = [this, i](QWidget *target, Qt::Edge edge) {
                attachDrop(i, target, edge);
            };
            nativeMenus->add({panel.name, [this, i] { return panels[i].session->floating(); },
                [this, i] { togglePanelPlacement(i); }, [this,i] { dockNative(i); },
                [this,i] { return panels[i].session->active(); },
                [this,i] { return panels[i].session->interfaceScale(); },
                [this,i](double factor) { panels[i].session->setInterfaceScale(factor); }});
            panel.session->setChrome(static_cast<Unbound::WindowChrome>(style->currentIndex()));
            panel.session->setConstrained(constrain->isChecked());
            panel.session->setSnapping(snap->isChecked());
            const auto row=managerView->addPanel(i,panel.name,panel.label);
            panel.undock=row.undock; panel.dock=row.dock; panel.show=row.show;
            panel.hide=row.hide; panel.expand=row.expand; panel.center=row.center;
            if(panel.name=="unboundWorkspaceToolbar")for(auto button:{panel.undock,panel.dock,panel.expand,panel.center})button->hide();
            managerView->addScale(i,new Unbound::PanelScaleControls(this,
                [this,i] { return panels[i].session->interfaceScale(); },
                [this,i](double factor) { panels[i].session->setInterfaceScale(factor); },
                [this,i] { return panels[i].session->active(); }));
            panel.session->changed = [this, i] {
                if (restoringNative) { updateRow(i); return; }
                if (!panels[i].session->active()) expansionController->sessionInactive(i);
                if (!quitting) panels[i].desired = panels[i].session->active() && !panels[i].session->dockPending();
                if (auto group = groupFor(i)) group->syncVisibility();
                nativeWorkspace->queueRestore(page());
                nativeWorkspace->refreshNativeTargets(page());
                syncNativeToggles();
                if (viewerMode && page()) viewerMode->refresh(page()->window());
                updateRow(i); updateNativeSizing(); readyNativeDrag(i); if (!quitting) saveTimer.start();
            };
            panel.session->expandedClosed = [this, i] { if (expansionController->index() == i) restoreExpanded(); };
            panel.session->arrangementChanged = [this] { if (!quitting) saveTimer.start(); };
            connect(panel.undock, &QPushButton::clicked, this, [this, i] { action("undock", panels[i].label); if (nativeWorkspace->contains(panels[i].name)) nativeWorkspace->detach(panels[i].name); else requestUndock(i); });
            connect(panel.dock, &QPushButton::clicked, this, [this, i] { action("dock", panels[i].label); dockOne(i); });
            connect(panel.show, &QPushButton::clicked, this, [this, i] { action("show", panels[i].label); showPanel(i,true); });
            connect(panel.hide, &QPushButton::clicked, this, [this, i] { action("hide", panels[i].label); showPanel(i,false); });
            connect(panel.expand, &QPushButton::clicked, this, [this, i] { toggleExpanded(i); });
            connect(panel.center, &QPushButton::clicked, this, [this, i] { action("center", panels[i].label); panels[i].session->centerOnCurrentDisplay(); });
            updateRow(i);
        }
        const auto nativePlanError=nativeWorkspace->setPending(settings.data["native_arrangement"].toObject(),pendingGroups);
        if (!nativePlanError.isEmpty()) status->setText(nativePlanError);
        for (auto &panel : panels) if (nativeWorkspace->requiredPanels().contains(panel.name)) panel.desired=true;
        footer = new Unbound::FooterPanel(this, []() -> QWidget * {
            for (auto window : QApplication::topLevelWidgets()) if (window->objectName() == QLatin1String("UiMainWindow")) return window;
            return nullptr;
        }, settings.data["footer"].toObject());
        footer->changed = [this] { saveTimer.start(); };
        footer->primarySurface=[this]() -> QWidget * {
            if (!page() || !page()->isVisible()) return nullptr;
            // One primary owner, independent of panel location, focus or size.
            // Detached groups never take the workspace bands with them.
            return Unbound::resolveMainWindow();
        };
        footer->surfaceHosts=[this] {
            QList<QWidget *> hosts;
            if(page() && page()->isVisible())if(auto main=Unbound::resolveMainWindow())hosts.append(main);
            return hosts;
        };
        workspaceHeader=new Unbound::PinnedWorkspaceHeader(this,settings.data["header"].toObject());
        workspaceHeader->page=[this] {return page();};
        workspaceHeader->toolbar=[this] {return Unbound::panelSurface(page(),"unboundWorkspaceToolbar");};
        workspaceHeader->primary=footer->primarySurface;
        workspaceHeader->prepareNativeToolbar=[this]() -> QString {
            for(int i=0;i<panels.size();++i)if(panels[i].name=="unboundWorkspaceToolbar" && panels[i].session->active()) {
                const auto error=panels[i].session->dock();if(!error.isEmpty())return error;
                panels[i].desired=false;
            }
            return {};
        };
        workspaceHeader->changed=[this] {saveTimer.start();if(headers)headers->schedule();};
        workspaceHeader->failed=[this](QString error) {status->setText(error);};
        expansionController->contentBounds=[this](QWidget *owner) {
            return Unbound::workspaceViewportBounds(owner,owner==Unbound::resolveMainWindow()?page():nullptr);
        };
        workspaceHeader->geometryChanged=[this]{expansionController->refreshGeometry();};
        footer->geometryChanged=[this]{expansionController->refreshGeometry();};
        expansionController->presentationChanged=[this]{footer->refreshSurfaces();workspaceHeader->schedule();};
        workspaceHeader->schedule();
        footer->placementChanged=[this](bool available,bool pinned) {
            if(!managerView->pinFooter)return;
            managerView->pinFooter->setEnabled(available);managerView->pinFooter->setChecked(pinned);
        };
        viewerMode = new Unbound::ViewerModeCommand(this,
            [this] { return page() && page()->isVisible() && panels[2].session->active() && panels[3].session->active(); },
            [this](bool single) {
                if (nativeWorkspace->contains(panels[2].name)) { nativeWorkspace->setVisible(panels[2].name,!single); return; }
                panels[2].session->setHiddenByUser(single, false);
                if (auto group = groupFor(2)) group->syncVisibility(single ? nullptr : panels[2].session);
            });
        toolbarDecoration = new Unbound::ToolbarDecoration(this);
        nativeDecoration = new Unbound::NativePanelDecoration(this);
        timelineControl = new Unbound::TimelineControl(this);
        nativeDrag = new Unbound::NativePanelDrag(this);
        callers = new Unbound::WorkspaceCallers(this);
        for (int index = 0; index < panels.size(); ++index) callers->add({QStringLiteral("unboundCaller_") + panels[index].name, panels[index].label,
            [this, index] { if(workspaceHeader && panels[index].name=="unboundWorkspaceToolbar")return workspaceHeader->shown();return panelShown(index); },
            [this, index] {
                if(workspaceHeader && panels[index].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(!workspaceHeader->shown());return;}
                queuePanelDoor(index);
            }, [this, index] { return layoutMode=="managed" || panels[index].toggle.isEmpty() || panels[index].session->active(); }});
        callers->wire();
        nativeToggles=new Unbound::NativePanelToggles(this);
        nativeToggles->blocked=[this]{return quitting || restoringNative || (workspaceRestore && workspaceRestore->active());};
        for (int i=0;i<panels.size();++i)
            nativeToggles->add(panels[i].toggle,[this,i] { return layoutMode=="managed" || panels[i].session->active(); },
                [this,i] { return panelShown(i); },[this,i] { queuePanelDoor(i); });
        commands=new Unbound::PanelCommandCatalog(this,[this] { return !quitting; });
        for (int i=0;i<panels.size();++i)if(panels[i].name!="frameEditToolbarTop")
            commands->addPanel(panels[i].name,panels[i].label,
                [this] { return page() && page()->isVisible(); },
                [this,i] { if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar")workspaceHeader->setShown(!workspaceHeader->shown());else queuePanelDoor(i); },
                [this,i] { toggleExpanded(i); });
        commands->add({"panel.expand.hovered","Maximize / Restore Panel Under Pointer (Backtick)",
            [this] { return page() && page()->isVisible() && hoveredExpansionTarget()>=0; },
            [this] { const int target=hoveredExpansionTarget(); if (target>=0) toggleExpanded(target); }});
        commands->defaultBinding("panel.expand.hovered","`");
        commands->addNavigation([this] { return footer && Unbound::resolveMainWindow(); },[this] {
            const auto error=footer->toggleVisibility();
            if (!error.isEmpty()) status->setText(error);
            return error;
        });
        auto editCommands=new Unbound::NativeEditCommands(this,[this] { return page(); });
        editCommands->audioExperiment=[this](int direction,bool skip){return audioExperiment && audioExperiment(direction,skip);};
        editCommands->editActive=[] { return Unbound::currentWorkspacePage(Unbound::resolveMainWindow())=="edit"; };
        editCommands->report=[this](QString message) { status->setText(message); };
        editCommands->addTo(commands);
        workspaceMenu = new Unbound::WorkspaceArrangements(this);
        auto commandMenu=new Unbound::PanelCommandMenu(this,commands);
        workspaceMenu->commandsMenu=[commandMenu](QMenu *root) {return commandMenu->menu(root);};
        workspaceMenu->showPanelShortcuts=[this] { commands->showEditor(); };
        auto panelCallers=new Unbound::PanelCallerMenu(this);
        panelCallers->currentPage=[] { return Unbound::currentWorkspacePage(Unbound::resolveMainWindow()); };
        for (int i=0;i<panels.size();++i) if(panels[i].name!="frameEditToolbarTop")panelCallers->add({panels[i].name,panels[i].label,"edit",
            [this] { return !quitting && page() && page()->isVisible(); },
            [this,i] {
                if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar")return workspaceHeader->shown();
                return panelShown(i);
            },
            [this,i] {
                if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(true);return;}
                showPanel(i,true);
            },[this,i](bool visible) {showPanel(i,visible);save();}});
        panelCallers->add({"page-navigation","Page Navigation","global",[this] { return !quitting && bool(footer); },
            [this] { return footer && !footer->snapshot()["hidden"].toBool(); },
            [this] { const auto error=footer->showPanel(); if (!error.isEmpty()) status->setText(error); },
            [this](bool visible) {status->setText(visible?footer->showPanel():footer->hidePanel());}});
        workspaceMenu->panelMenu=[panelCallers](QMenu *root) { return panelCallers->wire(root); };
        workspaceMenu->showPanelManager = [this] {
            showNormal(); Unbound::keepOnDisplay(this); raise(); activateWindow();
            Unbound::logEvent("controls.show", {{"origin","workspace-menu"}});
        };
        workspaceMenu->entries = [this] { return presetsUi->list(); };
        workspaceMenu->hiddenArrangements=[this]{return presetsUi->hasHidden();};
        workspaceMenu->currentPage = [] { return Unbound::currentWorkspacePage(Unbound::resolveMainWindow()); };
        workspaceMenu->nativeActive = [this] { return Unbound::currentWorkspacePage(Unbound::resolveMainWindow())!="edit" || layoutMode=="native"; };
        workspaceMenu->selected = [this] { return presets->currentData().toString(); };
        workspaceMenu->invoke = [this](const QString &operation, const QString &id) {
            if (operation == "Reorder") { showNormal(); raise(); activateWindow(); }
            presetsUi->invoke(operation, id);
        };
        workspaceMenu->nativeWorkspace = [this](const QString &activePage) {
            if (activePage == "edit") {
                Unbound::logEvent("workspace.native-return.request",{{"page",activePage}});
                ++bulkVersion;restoreExpanded();save();
                const auto previousMode=layoutMode;layoutMode="native";
                const auto error=dockAll();
                if(!error.isEmpty()) {
                    layoutMode=previousMode;status->setText(error);
                    Unbound::logEvent("workspace.native-return.failed",{{"page",activePage},{"error",error}});
                    return;
                }
                syncNativeToggles(true);
                Unbound::logEvent("workspace.native-return.complete",{{"page",activePage}});
            } else footer->refreshSurfaces();
            if (auto main = Unbound::resolveMainWindow()) {
                Unbound::setNativeBackgroundHidden(main,{},false);
                if(main->isMinimized())main->showNormal();else main->show();
                main->raise(); main->activateWindow();
            }
        };
        workspaceMenu->wire();
        toolbarDoors=new Unbound::ToolbarDoors(this);
        for (int i=0;i<panels.size();++i) if (panels[i].name=="widgetEditSourcePlayer" || panels[i].name=="widgetEditTimelinePlayer")
            toolbarDoors->add({panels[i].name,panels[i].label,
                [this,i] { return panelShown(i); },
                [this,i] { if (!quitting && !QApplication::activeModalWidget())togglePanelDoor(i); },
                [this,i](QPoint) {beginPanelDrag(i);}});
        headers=new Unbound::WorkspaceHeaders(this);
        for (int i=0;i<panels.size();++i) headers->add({panels[i].name,panels[i].label,
            [this,i]() -> QWidget * { return panels[i].session->active() && panels[i].session->window() ? panels[i].session->window()->window() : nullptr; },
            [this,i] { return !panels[i].session->hiddenByUser(); },
            [this,i](bool visible) { if (quitting) return;showPanel(i,visible);syncNativeToggles();save(); },
            [this,i] { return panels[i].session->interfaceScale(); },
            [this,i](double factor) { if (!quitting) { panels[i].session->setInterfaceScale(factor);save(); } },
            [this,i] { floatToolbar(i); }});
        // Header/Footer are direct visibility actions in the one Panels model.
        // Pinning is structural, not another ordinary navigation submenu.
        headers->surfaceControls=[](QMenu *,QWidget *){};
        headers->projectSurface=[] { return Unbound::resolveMainWindow(); };
        headers->primary=footer->primarySurface;
        headers->primaryHeader=[this]{return Unbound::panelSurface(page(),"unboundWorkspaceToolbar");};
        headers->panelEntries=[panelCallers](QMenu *menu) {panelCallers->populateFlat(menu);};
        headers->windows=[this] {
            QList<Unbound::WorkspaceHeaders::Window> result;
            for (auto group:groups) if (group->active()) {
                QPointer<Unbound::PanelGroup> alive=group;const auto names=group->snapshot()["panels"].toArray();
                QString label;for (const auto &panel:panels) if (!names.isEmpty() && panel.name==names.first().toString()) label=panel.label;
                if (names.size()>1) label+=QString(" +%1 panels").arg(names.size()-1);
                result.append({label,[alive] { return alive && alive->window() && alive->window()->isVisible(); },[this,alive](bool visible) {
                    if (quitting || !alive) return;alive->setWindowHidden(!visible);
                    if (visible && !alive->window()->isVisible()) for (int i=0;i<panels.size();++i) if (alive->contains(panels[i].session)) { showPanel(i,true);break; }save();
                }});
            }
            for (int i=0;i<panels.size();++i) if (panels[i].session->active() && !panels[i].session->grouped())
                result.append({panels[i].label,[this,i] { return !panels[i].session->hiddenByUser(); },[this,i](bool visible) { if (!quitting) { showPanel(i,visible);save(); } }});
            return result;
        };
        headers->schedule();
        managerView->addFooter();
        connect(managerView->showFooter,&QPushButton::clicked,this,[this] { status->setText(footer->showPanel()); });
        connect(managerView->hideFooter,&QPushButton::clicked,this,[this] { status->setText(footer->hidePanel()); });
        connect(managerView->pinFooter,&QPushButton::clicked,this,[this](bool pinned) { status->setText(footer->pinPrimarySurface(pinned)); });
        pageLifecycle=new Unbound::EditPageLifecycle(this);
        pageLifecycle->running=[this] { return !quitting; };
        pageLifecycle->checkpoint=[this] { restoreExpanded(); saveTimer.stop(); save(); };
        pageLifecycle->suspend=[this] {
            workspaceRestore->cancel();
            for (auto &panel:panels) { ++panel.requestVersion; panel.pending=false; }
            nativePresentation.restore(); nativeSizing->restore();
            if (auto main=Unbound::resolveMainWindow()) if (main->property("unboundBackgroundHidden").toBool())
                Unbound::setNativeBackgroundHidden(main,{},false);
            if (footer) footer->refreshSurfaces();
        };
        pageLifecycle->resume=[this] {
            for (auto group:groups) if (group->active()) group->syncVisibility();
            for (auto &panel:panels) if (panel.session->active()) panel.session->resumePagePresentation();
            if (footer) footer->restore();
            updateNativeSizing();
            if (background->isChecked()) applyBackgroundVisibility(true);
        };
        saveTimer.setSingleShot(true); saveTimer.setInterval(250);
        connect(&saveTimer, &QTimer::timeout, this, [this] { save(); });
        restoreTimer.setSingleShot(true); restoreTimer.setInterval(250);
        connect(&restoreTimer, &QTimer::timeout, this, [this] { restore(); });
        nativeNavigation=new Unbound::NativePanelNavigation(this);
        nativeNavigation->shortcutOwners=[this](const QKeySequence &key){return commands->nativeShortcutOwners(key);};
        nativeNavigation->context=[this](const QString &id) {
            Unbound::NativePanelNavigation::Context state;
            state.page=page();state.version=bulkVersion;
            if(quitting)state.blocked="quitting";
            else if(layoutMode!="managed")state.blocked="native-mode";
            else if(restoringNative)state.blocked="native-restoration";
            else if(workspaceRestore->savingBlocked())state.blocked="workspace-checkpoint-blocked";
            else if(!state.page || !state.page->isVisible())state.blocked="page-not-visible";
            else if(Unbound::visibleCinemaViewer())state.blocked="cinema-viewer";
            for(int i=0;i<panels.size();++i)if(panels[i].session->active()) {
                if(panels[i].name==id){state.destination=panels[i].session;state.surface=panels[i].session->window();state.shown=panelShown(i);}
                if(panels[i].name=="frameEditPageMediaPool")state.pool=panels[i].session->window();
            }
            return state;
        };
        nativeNavigation->reveal=[this](QObject *source) {
            for(int i=0;i<panels.size();++i)if(panels[i].session==source){showPanel(i,true,false);save();break;}
        };
        nativeNavigation->scanActions(Unbound::resolveMainWindow());
        qApp->installEventFilter(this);
        installEventFilter(this);
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { shutdown(); });
        connect(qApp, &QGuiApplication::screenRemoved, this, [this] { keepGripsReachable(); });
        auto watchScreen = [this](QScreen *screen) {
            connect(screen, &QScreen::availableGeometryChanged, this, [this] { if (constrain->isChecked()) keepGripsReachable(); });
        };
        for (auto screen : QGuiApplication::screens()) watchScreen(screen);
        connect(qApp, &QGuiApplication::screenAdded, this, [this, watchScreen](QScreen *screen) { watchScreen(screen); keepGripsReachable(); });
        const QRect position = Unbound::WorkspaceSettings::rectangle(settings.data["controls"].toObject());
        if (position.isValid()) setGeometry(position);
        restoreTimer.start();
        wireNativeToggles();
    }
    bool invokeWorkflowCommand(const QString &id) { return !quitting && commands && commands->invoke(id,"resolve-workflow-shortcut"); }
    ~PanelControls() override
    {
        shutdown();
        qDeleteAll(groups); groups.clear();
        delete nativeMenus;
        qApp->removeEventFilter(this);
        for (auto &panel : panels) { panel.session->expandedClosed = {}; panel.session->changed = {}; panel.session->arrangementChanged = {}; panel.session->attachRequested = {}; panel.session->beforeLayoutTransfer = {}; }
    }
protected:
    void closeEvent(QCloseEvent *event) override
    {
        action("hide-controls"); save(); event->accept(); hide();
    }
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (quitting) return false;
        nativeNavigation->observe(object,event);
        if (expansionController->index() >= 0 && event->type() == QEvent::Hide && object == page()) restoreExpanded();
        if (object == this && (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::WindowTitleChange)) QTimer::singleShot(0, this, [this] {
            QPointer<PanelControls> alive(this);
            Unbound::setNativePin(this, pin->isChecked(), [alive](bool value) { if (alive) alive->pin->setChecked(value); });
        });
        if (object == this && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) saveTimer.start();
        if (event->type() == QEvent::Show) {
            if(auto shown=qobject_cast<QWidget *>(object))
                if(shown->objectName()==QLatin1String("WStackPage_Conform"))editPageCache=shown;
            if (!toggleScanQueued) {
                toggleScanQueued = true;
                QTimer::singleShot(0, this, [this] { toggleScanQueued = false; wireNativeToggles(); });
            }
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && (widget->objectName() == QLatin1String("UiMainWindow") ||
                widget->objectName() == QLatin1String("WStackPage_Conform"))) restoreTimer.start();
            else if (widget) for (const auto &panel : panels)
                if (panel.desired && !panel.pending && !panel.session->active() &&
                    (widget->objectName() == panel.name || (!Unbound::nativeWorkspacePanelAlias(panel.name).isEmpty() &&
                     widget->objectName() == Unbound::nativeWorkspacePanelAlias(panel.name))))
                    restoreTimer.start();
        }
        return false;
    }
private:
    int hoveredExpansionTarget() const {
        if (expansionController->index()>=0) return expansionController->index();
        QList<Unbound::PanelHoverSurface> surfaces;
        for (int i=0;i<panels.size();++i)
            surfaces.append({i,panels[i].name,panels[i].session->active() ? panels[i].session->window() : Unbound::panelSurface(page(),panels[i].name)});
        return Unbound::panelHoverTarget(QApplication::widgetAt(QCursor::pos()),surfaces);
    }
    void showPanel(int i,bool visible,bool activate=true)
    {
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(visible);return;}
        if (visible) if (auto group=groupFor(i)) if (group->windowHidden()) group->setWindowHidden(false,activate);
        if (!panels[i].session->active()) {
            auto parked=Unbound::panelSurface(page(),panels[i].name);
            if(visible && nativeWorkspace->active() && Unbound::NativeRootLayout::ownsParkedWidget(parked,page())) {
                // Reuse the existing panel lease and add operation. The parked
                // native page never becomes a visible second workspace.
                parked->show();pendingPrimaryPanels.insert(i);requestUndock(i);return;
            }
            bool handled=Unbound::setNativePanelVisibility(Unbound::resolveMainWindow(),panels[i].toggle,visible);
            QPointer<QWidget> target=Unbound::panelSurface(page(),panels[i].name);
            if(!handled && panels[i].toggle.isEmpty() && target &&
                !(panels[i].name=="frameEditToolbarTop" && managedOwnerFor(target))) {
                if(visible && panels[i].name=="widgetEditSourcePlayer") {
                    Unbound::prepareSourceViewer(page(),target);target=Unbound::panelSurface(page(),panels[i].name);
                }
                if(target && page() && page()->isAncestorOf(target))handled=nativeSurfaceDoors.set(panels[i].name,target,visible);
            }
            if (!handled && panels[i].name=="frameEditToolbarTop" && managedOwnerFor(target)) {
                if (visible) showPanel(1,true);
                if(visible) {
                    QWidget *timeline=nullptr;
                    for(auto ancestor=target->parentWidget();ancestor;ancestor=ancestor->parentWidget())
                        if(ancestor->objectName()=="frameConformTimelines"){timeline=ancestor;break;}
                    handled=target->isVisible() || Unbound::revealAttachedTimelineToolbar(page(),target,timeline);
                } else {target->hide();handled=true;}
            }
            Unbound::logEvent("panel.visibility.native-request",{{"panel",panels[i].name},{"requested",visible},{"handled",handled}});
            if(!handled)status->setText(QStringLiteral("Native panel is not available: ")+panels[i].label);
            return;
        }
        if (nativeWorkspace->contains(panels[i].name)) nativeWorkspace->setVisible(panels[i].name,visible);
        else {
            panels[i].session->setHiddenByUser(!visible,activate);
            if (auto group=groupFor(i)) group->syncVisibility(visible ? panels[i].session : nullptr);
        }
    }
    void floatToolbar(int i)
    {
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(true);return;}
        if (quitting || QApplication::activeModalWidget() || !page() || !page()->isVisible()) return;
        const bool pinned=panels[i].saved["toolbar_float_pinned"].toBool(true);
        if (auto group=groupFor(i)) group->detachMember(panels[i].session);
        else if (nativeWorkspace->contains(panels[i].name)) nativeWorkspace->detach(panels[i].name);
        else if (!panels[i].session->active()) { requestUndock(i);return; }
        panels[i].session->setHiddenByUser(false);
        panels[i].session->setPinned(pinned);save();
        Unbound::logEvent("toolbar.placement",{{"state","floating"},{"panel",panels[i].name}});
    }
    void toggleWorkspaceToolbar(int i)
    {
        if(workspaceHeader){workspaceHeader->setShown(!workspaceHeader->shown());return;}
        if (quitting || QApplication::activeModalWidget() || !page() || !page()->isVisible()) return;
        if (!panels[i].session->active() || panels[i].session->grouped() || nativeWorkspace->contains(panels[i].name)) { floatToolbar(i);return; }
        if (panels[1].session->active()) {
            status->setText(combinePanels(i,1,Qt::Vertical,true));save();
            Unbound::logEvent("toolbar.placement",{{"state","attached"},{"panel",panels[i].name}});
        } else status->setText("Show Timeline before attaching the Workspace Toolbar, or drag it onto another window.");
    }
    void enterManagedFromGesture(const QString &panel) {
        if(layoutMode=="managed")return;
        layoutMode="managed";
        if(presets){const QSignalBlocker block(presets);presets->setCurrentIndex(qMax(0,presets->findData(QString())));}
        Unbound::logEvent("workspace.mode.enter",{{"page","edit"},{"mode","managed"},{"origin","panel-gesture"},{"panel",panel},{"default_loaded",false}});
    }
    void togglePanelPlacement(int i)
    {
        if(panels[i].name=="frameEditToolbarTop")return;
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(!workspaceHeader->shown());return;}
        enterManagedFromGesture(panels[i].name);
        if (nativeWorkspace->contains(panels[i].name)) { nativeWorkspace->detach(panels[i].name); readyNativeDrag(i); save(); }
        else if(auto group=groupFor(i)){group->detachMember(panels[i].session);save();}
        else if(panels[i].session->active() && nativeWorkspace->active())dockNative(i);
        else if (panels[i].session->active()) dockOne(i);
        else requestUndock(i);
    }
    QWidget *managedOwnerFor(QWidget *target) const {
        auto root=page();
        for (const auto &owner:panels) if (owner.session->active() && owner.session->nativeOwnerWindow()==(root ? root->window() : nullptr) &&
            Unbound::panelInManagedScope(target,root,owner.session->window()) && !root->isAncestorOf(target)) return owner.session->window();
        return nullptr;
    }
    void dockNative(int i,QWidget *target=nullptr,Qt::Edge edge=Qt::RightEdge)
    {
        if (quitting || QApplication::activeModalWidget() || !page() || !page()->isVisible()) return;
        restoreExpanded(); auto group=groupFor(i);
        const auto direction=edge==Qt::Edge(0) ? Qt::Orientation(0) : edge==Qt::TopEdge || edge==Qt::BottomEdge ? Qt::Vertical : Qt::Horizontal;
        const auto error=nativeWorkspace->dockSelection(page(),panels[i].name,group,
            target ? nativeWorkspace->targetName(target) : nativeWorkspace->defaultDockTarget(),direction,edge==Qt::LeftEdge || edge==Qt::TopEdge);
        if (!error.isEmpty() && group && group->active()) {
            group->setChrome(static_cast<Unbound::WindowChrome>(style->currentIndex())); group->setSnapping(snap->isChecked());
        }
        status->setText(error.isEmpty() ? QStringLiteral("Panel arrangement docked in the native workspace.") : error);
        Unbound::logEvent("workspace.native-command.complete",{{"panel",panels[i].name},{"success",error.isEmpty()},{"error",error}});
        for (int j=0;j<panels.size();++j) updateRow(j);
        nativeWorkspace->refreshNativeTargets(page());
        save();
    }
    bool panelShown(int i) const {
        if(nativeWorkspace->contains(panels[i].name))return nativeWorkspace->selected(panels[i].name);
        auto session=panels[i].session;
        if(session->active()) {
            auto group=groupFor(i);
            return !session->hiddenByUser() && (!group || (!group->windowHidden() && group->selected(session))) && session->window() && session->window()->isVisible();
        }
        auto target=Unbound::panelSurface(page(),panels[i].name);return target && target->isVisible();
    }
    void beginPanelDrag(int i) {
        if(quitting || QApplication::activeModalWidget() || !page() || !page()->isVisible())return;
        enterManagedFromGesture(panels[i].name);restoreExpanded();nativeDrag->begin(panels[i].name);
        if(nativeWorkspace->contains(panels[i].name))nativeWorkspace->detach(panels[i].name);
        else if(auto group=groupFor(i))group->detachMember(panels[i].session);
        if(!panels[i].session->active())requestUndock(i);
        readyNativeDrag(i);
    }
    void togglePanelDoor(int i)
    {
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){workspaceHeader->setShown(!workspaceHeader->shown());return;}
        auto session = panels[i].session;
        if(!page() || !page()->isVisible())return;
        if(!session->active()){showPanel(i,!panelShown(i));save();return;}
        if (expansionController->index() >= 0) restoreExpanded();
        auto group = groupFor(i);
        const bool reveal = session->hiddenByUser() || (group && (group->windowHidden() || !group->selected(session))) ||
            (nativeWorkspace->contains(panels[i].name) && !nativeWorkspace->selected(panels[i].name));
        showPanel(i,reveal);
        syncNativeToggles();
        Unbound::logEvent(QStringLiteral("panel.visibility.native-toggle"), {{"panel", panels[i].name}, {"visible", reveal}});
    }
    void queuePanelDoor(int i)
    {
        if (quitting || restoringNative || workspaceRestore->active() || !page() || !page()->isVisible() || pendingNativeDoors.contains(i)) return;
        pendingNativeDoors.insert(i);
        const auto version=bulkVersion;
        QTimer::singleShot(0, this, [this, i, version] {
            pendingNativeDoors.remove(i);
            if(!quitting && !restoringNative && !workspaceRestore->active() && version==bulkVersion)togglePanelDoor(i);
        });
    }
    void syncNativeToggles(bool nativeSnapshot=false) { if (nativeToggles) nativeToggles->sync(page(),nativeSnapshot); }
    void wireNativeToggles() { if (!quitting) nativeToggles->wire(page()); }
    QJsonObject arrangement() const
    {
        auto result = settings.pageView("edit");
        result["page"] = "edit";
        result.remove(QStringLiteral("controls"));
        result.remove(QStringLiteral("selected_arrangement"));
        return result;
    }
    void updatePresetState()
    {
        if (presetsUi) presetsUi->updateState(arrangement());
        if (workspaceMenu) workspaceMenu->wire();
    }
    void restoreArrangement()
    {
        workspaceRestore->preset(presetsUi->find(presets->currentData().toString()),page() && page()->isVisible());
    }
    QString applyWorkspace(QJsonObject value)
    {
        value=Unbound::migrateWorkspaceHeader(Unbound::fitStarterWorkspace(value));
        if(value.contains("selected_arrangement")) {
            const QSignalBlocker block(presets);presets->setCurrentIndex(qMax(0,presets->findData(value["selected_arrangement"].toString())));
        }
        layoutMode=Unbound::PageWorkspaceState::mode(value);
        pendingMainPlacement=value["main_window"].toObject();mainPlacementRecord=pendingMainPlacement;
        if (footer) footer->setState(value["footer"].toObject());
        if(workspaceHeader)workspaceHeader->setState(value["header"].toObject());
        { const QSignalBlocker blocker(background); background->setChecked(false); }
        pendingGroups = value["groups"].toArray();
        const auto nativeError=nativeWorkspace->setPending(value["native_arrangement"].toObject(),pendingGroups);
        if (!nativeError.isEmpty()) return nativeError;
        pin->setChecked(value["pin_controls"].toBool());
        constrain->setChecked(value["keep_on_screen"].toBool(true));
        snap->setChecked(value["snap_windows"].toBool(true));
        hierarchyOption->setChecked(value["panel_hierarchy"].toBool(true));
        style->setCurrentIndex(qBound(0, value["window_chrome"].toInt(), 2));
        const auto saved = value["panels"].toObject();
        for (auto &panel : panels) {
            panel.saved = saved[panel.name].toObject();
            panel.session->setInterfaceScale(panel.saved["interface_scale"].toDouble(1.0));
            panel.desired = panel.saved["floating"].toBool() || nativeWorkspace->requiredPanels().contains(panel.name);
        }
        return {};
    }
    static void action(const QString &name, const QString &panel = {})
    { Unbound::logEvent(QStringLiteral("button.click"), {{"action", name}, {"panel", panel}}); }
    QWidget *page() const
    {
        if(editPageCache)return editPageCache;
        // Edit is constructed lazily during application startup. A missing page
        // must not cause a whole-tree search for each new native widget event.
        if(pageScanClock.isValid() && pageScanClock.elapsed()<100)return nullptr;
        pageScanClock.restart();
        for(QWidget *window:QApplication::topLevelWidgets())
            if(window->objectName()==QLatin1String("UiMainWindow")) {
                editPageCache=window->findChild<QWidget *>(QStringLiteral("WStackPage_Conform"));
                return editPageCache;
            }
        return nullptr;
    }
    void readyNativeDrag(int i)
    {
        if (!nativeDrag || !panels[i].session->active() || panels[i].session->grouped()) return;
        nativeDrag->ready(panels[i].name,static_cast<Unbound::WorkspaceFloatingHost *>(panels[i].session->window()));
    }
    void updateRow(int i)
    {
        auto &p = panels[i]; const bool active = p.session->active();
        optionsScroll->preservePosition([&] {
            p.undock->setEnabled(p.name!="frameEditToolbarTop" && (!active || p.session->placement()==Unbound::PanelPlacement::NativeSurface) && !p.pending);
            p.dock->setEnabled(active && !p.session->dockPending());
            p.dock->setText(p.session->dockPending() ? QStringLiteral("Pending") : QStringLiteral("Dock"));
            const bool nativeCaller=!p.toggle.isEmpty() && page() && page()->isVisible();
            p.show->setEnabled(active || nativeCaller); p.hide->setEnabled(active ? !p.session->hiddenByUser() : nativeCaller);
            p.expand->setText(expansionController->index() == i ? QStringLiteral("Restore Size") : QStringLiteral("Expand"));
            p.expand->setEnabled(true);
            p.center->setEnabled(p.session->floating());
        });
    }
    void toggleExpanded(int i)
    {
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar"){status->setText("Workspace Toolbar is pinned; use Show or Hide.");return;}
        action(QStringLiteral("expand-toggle"),panels[i].label);
        if (expansionController->index()==i) { restoreExpanded(); return; }
        restoreExpanded();
        if (!page() || !page()->isVisible() || QApplication::activeModalWidget()) {
            status->setText(QStringLiteral("Open the Edit page and close dialogs before expanding a panel.")); return;
        }
        // Restore needs a current in-memory snapshot, not disk persistence or
        // menu rewiring before the first visible maximize result.
        persistence.capturePanels(persistenceSources());
        saveTimer.start();
        auto &panel=panels[i];
        Unbound::PanelExpansionController::Target target;
        target.index=i; target.name=panel.name; target.session=panel.session; target.group=groupFor(i);
        target.workspace=panel.session->grouped()?panel.session->window()->window():
            (footer->primarySurface?footer->primarySurface():nullptr);
        if(!target.workspace)target.workspace=page()->window();
        target.saved=panel.saved; target.native=nativeWorkspace->contains(panel.name) ? nativeWorkspace->saved() : QJsonObject();
        target.pending=[this,i] { return panels[i].pending; };
        target.undock=[this,i] { requestUndock(i,true); };
        target.cancelPending=[this,i] { ++panels[i].requestVersion; panels[i].pending=false; };
        target.restoreSaved=[this,i](QJsonObject value) { panels[i].saved=value; };
        target.restoreNative=[this](QJsonObject value) { return nativeWorkspace->restorePresentation(value); };
        target.restored=[this,i] { panels[i].desired=panels[i].session->active() && !panels[i].session->dockPending(); };
        expansionController->begin(std::move(target),page());
    }
    void restoreExpanded() { expansionController->restore(); }
    void outcome(const QString &operation, int i, const QString &error)
    {
        Unbound::logEvent(QStringLiteral("button.result"), {{"action", operation}, {"panel", panels[i].label},
            {"success", error.isEmpty()}, {"error", error}});
        status->setText(error.isEmpty() ? panels[i].label + (operation == "dock" ?
            (panels[i].session->dockPending() ? " will dock when Edit returns." : " returned to Resolve.") : " is floating.")
            : panels[i].label + ": " + error);
    }
    void requestUndock(int i, bool restoring = false)
    {
        if(panels[i].name=="frameEditToolbarTop"){panels[i].desired=false;return;}
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar" && !(restoring && nativeWorkspace->requiredPanels().contains(panels[i].name))){if(!restoring)workspaceHeader->setShown(true);workspaceHeader->schedule();return;}
        if (quitting) return;
        if (QApplication::activeModalWidget()) return;
        auto &p = panels[i];
        if (p.name=="unboundWorkspaceToolbar" && !p.saved.contains("toolbar_float_pinned")) p.saved["toolbar_float_pinned"]=true;
        if (p.pending || p.session->active()) return;
        if (!restoring) { enterManagedFromGesture(p.name);p.saved["hidden"] = false; }
        QWidget *root = page();
        QWidget *target = Unbound::panelSurface(root, p.name);
        if (!restoring && p.name=="frameEditToolbarTop" && panels[1].session->active() && target && !target->isVisible()) {
            showPanel(1,true);
            if (Unbound::revealAttachedTimelineToolbar(root,target,Unbound::panelSurface(root,"frameConformTimelines"))) {
                updateRow(i); save(); return;
            }
        }
        if(target && Unbound::NativeRootLayout::ownsParkedWidget(target,root)) {
            Unbound::setNativePanelVisibility(Unbound::resolveMainWindow(),p.toggle,true);
            finishUndock(i);return;
        }
        if (nativeSizing && expansionController->index()<0 && root && root->isVisible()) nativeSizing->prepare(root->window(),root);
        if (p.name == QLatin1String("widgetEditSourcePlayer") && root && root->isVisible() &&
            Unbound::prepareSourceViewer(root, target)) {
            p.pending = true; updateRow(i);
            const unsigned request = ++p.requestVersion;
            awaitUndockSurface(i, request, root);
            return;
        }
        if (root && root->isVisible() && ((!target && restoring) || (target && !target->isVisible())) && !p.toggle.isEmpty()) {
            // The pinned toolbar may be a sibling of the page or leased to
            // its main surface. Use the existing main-window control resolver.
            if (Unbound::setNativePanelVisibility(root->window(),p.toggle,true,target)) {
                // Acquire while native activation has made the surface ready.
                // A queued native visibility update can hide Index again before
                // the first deferred readiness poll, leaving startup half-built.
                target=Unbound::panelSurface(root,p.name);
                if(target && target->isVisible()){finishUndock(i);return;}
                p.pending = true; updateRow(i);
                const unsigned request = ++p.requestVersion;
                awaitUndockSurface(i, request, root,restoring?10000:1000);
                return;
            }
        }
        finishUndock(i);
    }
    void awaitUndockSurface(int i, unsigned request, QWidget *root,int timeoutMs=1000)
    {
        new Unbound::SurfaceReadiness(this,root,
            [this,i,request] { return !quitting && panels[i].requestVersion==request; },
            [this,i] {
                auto surface=Unbound::panelSurface(page(),panels[i].name);
                if (panels[i].name==QLatin1String("widgetEditSourcePlayer")) surface=Unbound::sourceViewerSurface(surface);
                return page() && page()->isVisible() && surface && surface->isVisible();
            }, [this,i,request](bool ready) {
                if (quitting || panels[i].requestVersion!=request) return;
                Unbound::logEvent("panel.undock.readiness",{{"panel",panels[i].name},{"ready",ready}});
                panels[i].pending=false;
                if(!ready) {
                    const auto error=panels[i].label+": native surface did not become visible.";
                    outcome("undock",i,error);updateRow(i);
                    if(!workspaceRestore->failPreset(error))status->setText(error);
                    return;
                }
                finishUndock(i);
            },timeoutMs);
    }
    void finishUndock(int i)
    {
        if (quitting) return;
        auto &p = panels[i]; QWidget *root = page();
        QWidget *target = Unbound::panelSurface(root, p.name);
        Unbound::logEvent(QStringLiteral("panel.undock.preconditions"), {{"panel", p.name},
            {"page_found", bool(root)}, {"page_visible", root && root->isVisible()}, {"target_found", bool(target)},
            {"target_visible", target && target->isVisible()}, {"target_in_page", root && target && root->isAncestorOf(target)}});
        if (p.name == QLatin1String("widgetEditSourcePlayer")) target = Unbound::sourceViewerSurface(target);
        const QSize nativeSize = target ? target->size() : QSize();
        QWidget *managedOwner=managedOwnerFor(target);
        Unbound::logEvent("panel.undock.scope",{{"panel",p.name},{"managed_owner",Unbound::widgetState(managedOwner)},
            {"accepted",Unbound::panelInManagedScope(target,root,managedOwner)}});
        const QString error = p.contents ? p.session->undockContents(target, root, p.label,managedOwner) :
            p.session->undock(target, root, p.label, p.name == QLatin1String("widgetEditSourcePlayer"),false,managedOwner);
        if (error.isEmpty()) {
            groupReserve->request(p.session->nativeOwnerWindow());
            p.session->window()->setObjectName(QStringLiteral("unboundFloating_") + p.name);
            p.desired = true;
            if (p.label.endsWith("Toolbar")) {
                auto frame = p.session->floatingGeometry(); frame.setSize(QSize(qMax(320, nativeSize.width()), qMax(64, nativeSize.height()+28)));
                p.session->setFloatingGeometry(frame);
            }
            const QRect position = Unbound::WorkspaceSettings::rectangle(p.saved);
            if (position.isValid()) p.session->setFloatingGeometry(position);
            p.session->restoreWindowMaximized(p.saved["maximized"].toBool());
            if(p.saved.contains("display")&&Unbound::DisplayPlacement::match(p.saved,Unbound::workspaceDisplays())<0)
                p.saved["display_fallback"]=Unbound::DisplayPlacement::rect(p.session->floatingGeometry());
            p.session->setInterfaceScale(p.saved["interface_scale"].toDouble(1.0));
            p.session->setPinned(p.name=="unboundWorkspaceToolbar" ? p.saved["toolbar_float_pinned"].toBool(true) : p.saved["pinned"].toBool());
            if (p.saved["hidden"].toBool()) p.session->setHiddenByUser(true);
            if (background->isChecked()) QTimer::singleShot(0, this, [this] {
                if (!quitting && background->isChecked()) applyBackgroundVisibility(true);
            });
        }
        if (error.isEmpty() && p.name=="frameConformTimelines" && footer) footer->refreshSurfaces();
        if(pendingPrimaryPanels.remove(i) && error.isEmpty() && nativeWorkspace->active()) {
            const auto current=nativeWorkspace->saved()["panels"].toObject();
            const auto target=current.contains("widgetEditInspector")?QStringLiteral("widgetEditInspector"):current.keys().value(0);
            const auto attached=nativeWorkspace->dock(root,{{p.name,p.session}},{{"panel",p.name}},target,Qt::Vertical,false);
            Unbound::logEvent("workspace.panel.reveal-from-parked",{{"panel",p.name},{"success",attached.isEmpty()},{"error",attached}});
            if(!attached.isEmpty())status->setText(attached);
        }
        outcome("undock", i, error);updateRow(i);updateNativeSizing();
        if(!error.isEmpty()&&workspaceRestore->failPreset(p.label+": "+error))return;
        if(error.isEmpty())for(const auto &next:panels)if(next.desired&&!next.session->active()) {
            restoreTimer.start(0);break;
        }
        // A first maximize from Native UI may need to create the floating
        // session. It uses the same deferred checkpoint as an existing session.
        if(expansionController->index()==i)saveTimer.start();else save();
    }
    void dockOne(int i)
    {
        if(workspaceHeader && panels[i].name=="unboundWorkspaceToolbar" && !workspaceHeader->release()){status->setText("Could not restore the workspace header.");return;}
        if (expansionController->index() == i) restoreExpanded();
        auto &p = panels[i];
        ++p.requestVersion; p.pending = false;
        const QString error = p.session->active() ? p.session->dock() : QString();
        if (error.isEmpty()) p.desired = false;
        outcome("dock", i, error); updateRow(i); if(!restoringNative)save();
    }
    QString releaseWorkspaceBands() {
        if(footer && !footer->releaseNative())return QStringLiteral("Could not restore page navigation before changing the workspace.");
        if(workspaceHeader && !workspaceHeader->release())return QStringLiteral("Could not restore the workspace header before changing the workspace.");
        return {};
    }
    QString dockAll(bool cancelRestore=true) {
        QString error;
        {
            // Intermediate session callbacks must not replace the last complete
            // layout. Persist one complete native result after all transfers.
            const QScopedValueRollback<bool> transaction(restoringNative,true);
            restoreTimer.stop();
            if (cancelRestore && workspaceRestore) workspaceRestore->cancel();
            error=releaseWorkspaceBands();
            if(error.isEmpty() && nativeWorkspace && !nativeWorkspace->release(true))
                error=QStringLiteral("The native layout changed; restoration was stopped.");
            if(error.isEmpty()) {
                nativePresentation.restore();
                if (nativeSizing) nativeSizing->restore();
                {const QSignalBlocker block(background);background->setChecked(false);}
                applyBackgroundVisibility(false);++groupVersion;pendingGroups={};
                for(int i=panels.size()-1;i>=0;--i)dockOne(i);
                for(const auto &panel:panels)if(panel.session->active()) {
                    error=QStringLiteral("Could not restore native panel: ")+panel.label;break;
                }
            }
        }
        if(error.isEmpty())error=nativeSurfaceDoors.restore();
        if(!error.isEmpty()){status->setText(error);return error;}
        for(auto &panel:panels)panel.saved.remove("native_hidden");
        saveTimer.stop();save();return {};
    }
    bool applyBackgroundVisibility(bool hidden)
    {
        auto root = page(); if (!root) return !hidden;
        QList<QWidget *> floating;
        for (auto widget : QApplication::topLevelWidgets()) if (widget->isVisible() && widget->objectName().startsWith("unboundFloating_")) floating.append(widget);
        if (hidden && floating.isEmpty()) { status->setText(QStringLiteral("Undock a panel before hiding the background.")); return false; }
        const bool success = Unbound::setNativeBackgroundHidden(root->window(), floating, hidden);
        Unbound::logEvent(QStringLiteral("background.visibility"), {{"hidden", hidden}, {"success", success}});
        if (!success) status->setText(QStringLiteral("Could not hide the background independently; it remains visible."));
        return success;
    }
    QString createGroup(int a, int b, Qt::Orientation orientation, const QJsonObject &saved = {}, const QRect &destination = {})
    {
        auto group = new Unbound::PanelGroup(this,groupReserve);
        const auto error = group->join(panels[a].session, panels[b].session, orientation,destination);
        if (!error.isEmpty()) { delete group; return error; }
        configureGroup(group);
        if (!saved.isEmpty()) group->restore(saved);
        return {};
    }
    void configureGroup(Unbound::PanelGroup *group)
    {
        Unbound::UiWorkTiming timing("group.configure",group->window());
        groups.append(group);
        group->sectionDetached = [this](Unbound::PanelGroup *section) { configureGroup(section); };
        group->setChrome(static_cast<Unbound::WindowChrome>(style->currentIndex()));
        group->setSnapping(snap->isChecked());
        group->attachRequested = [this, group](QWidget *target, Qt::Edge edge) {
            for (int i = 0; i < panels.size(); ++i) if (group->contains(panels[i].session)) { attachDrop(i, target, edge); return; }
        };
        group->changed = [this] {
            for (int i = 0; i < panels.size(); ++i) updateRow(i);
            syncNativeToggles();
            if (!quitting) saveTimer.start();
        };
        for (int i = 0; i < panels.size(); ++i) updateRow(i);
    }
    Unbound::PanelGroup *groupFor(int panel) const
    { for (auto group : groups) if (group->contains(panels[panel].session)) return group; return nullptr; }
    QString combinePanels(int a, int b, Qt::Orientation orientation, bool before = true, const QString &targetLeaf = {}, const QRect &destination = {})
    {
        if(panels[a].name=="unboundWorkspaceToolbar" || panels[b].name=="unboundWorkspaceToolbar")return QStringLiteral("Workspace Toolbar is pinned above the surface; use Show or Hide.");
        auto left = groupFor(a), right = groupFor(b);
        if (left && left == right) return QStringLiteral("Those panels already share a group.");
        if (right && left) return right->merge(left, orientation, before, targetLeaf);
        if (right) return right->add(panels[a].session, orientation, before, targetLeaf);
        if (left) return left->add(panels[b].session, orientation, !before);
        return before ? createGroup(a, b, orientation,{},destination) : createGroup(b, a, orientation,{},destination);
    }
    void attachDrop(int source, QWidget *target, Qt::Edge edge)
    {
        Unbound::UiWorkTiming timing("group.drop",target ? target->window() : nullptr);
        if (quitting || QApplication::activeModalWidget() || expansionController->index() >= 0 || !panels[source].session->active()) return;
        if (nativeWorkspace->ownsTarget(target)) { dockNative(source,target,edge); return; }
        const auto section=Unbound::dockSectionKey(target);
        for (int j = 0; j < panels.size(); ++j) if (j != source && panels[j].session->active() &&
            (panels[j].session->window() == target || panels[j].session->window()->window() == target ||
             (!section.isEmpty() && target->isAncestorOf(panels[j].session->window())))) {
            if (groupFor(source) && groupFor(source) == groupFor(j)) return;
            const auto frame = target->window()->frameGeometry();
            const auto error = combinePanels(source, j, edge == Qt::Edge(0) ? Qt::Orientation(0) : edge == Qt::LeftEdge || edge == Qt::RightEdge ? Qt::Horizontal : Qt::Vertical,
                edge == Qt::LeftEdge || edge == Qt::TopEdge, target->isWindow() ? QString() : section.isEmpty()?panels[j].name:section,target->window()->geometry());
            if (error.isEmpty()) if (auto group = groupFor(source)) group->fitFrame(frame);
            status->setText(error.isEmpty() ? QStringLiteral("Panels joined. Drag a member header to detach.") : error);
            Unbound::logEvent(QStringLiteral("group.drop.result"), {{"success", error.isEmpty()}, {"error", error}});
            save(); return;
        }
    }
    void releaseGroups()
    {
        ++groupVersion; pendingGroups = {};
        if(workspaceHeader && !workspaceHeader->release()){status->setText("Could not restore the workspace header.");return;}
        if (nativeWorkspace) nativeWorkspace->release();
        for (auto group : groups) group->dissolve();
    }
    void restoreGroups()
    {
        if (quitting || !page() || !page()->isVisible()) return;
        const bool requested = !pendingGroups.isEmpty();
        for (int index = pendingGroups.size() - 1; index >= 0; --index) {
            const auto saved = pendingGroups[index].toObject();
            const auto tree = Unbound::PanelGroup::savedTree(saved); QStringList names;
            if (!Unbound::groupTreeNames(tree, names)) { status->setText(QStringLiteral("Invalid saved group layout; left unchanged.")); continue; }
            QList<Unbound::ManagedPanelSession *> available;
            for (const auto &name : names) for (const auto &panel : panels)
                if (panel.name == name && panel.session->active() && !panel.session->grouped()) available.append(panel.session);
            if (available.size() != names.size()) continue;
            auto group = new Unbound::PanelGroup(this);
            const auto error = group->initialize(available, tree, Unbound::WorkspaceSettings::rectangle(saved));
            if (error.isEmpty()) { configureGroup(group); group->restore(saved); pendingGroups.removeAt(index); }
            else { delete group; Unbound::logEvent(QStringLiteral("group.restore.pending"), {{"error", error}}); }
        }
        if (requested) status->setText(pendingGroups.isEmpty() ? QStringLiteral("Panel groups restored.") : QStringLiteral("Some groups are waiting for unavailable panels."));
        workspaceRestore->finishPreset();save();
    }
    void quickExport()
    {
        auto root = page();
        if (root) for (auto item : root->window()->findChildren<QAction *>()) {
            QString text = item->text();
            text.remove('&'); text.remove(QChar(0x2026)); text.remove(QStringLiteral("..."));
            if ((text.trimmed().compare(QStringLiteral("Quick Export"), Qt::CaseInsensitive) == 0 ||
                item->objectName().compare(QStringLiteral("actionQuickExport"), Qt::CaseInsensitive) == 0) && item->isEnabled()) {
                Unbound::logEvent(QStringLiteral("quick-export.open.request"), {{"action", item->objectName()}});
                status->setText(QStringLiteral("Opening Resolve's Quick Export window."));
                item->trigger(); return;
            }
        }
        status->setText(QStringLiteral("Quick Export is not available. Open a timeline first."));
        Unbound::logEvent(QStringLiteral("quick-export.unavailable"));
    }
    void restore()
    {
        if (pageLifecycle) pageLifecycle->observe(page());
        if (footer) footer->restore();
        if (callers) callers->wire();
        if (nativeMenus) nativeMenus->wire();
        if (nativeDecoration && page()) for (int i=0; i<panels.size(); ++i) {
            if(panels[i].name=="unboundWorkspaceToolbar" || panels[i].name=="frameEditToolbarTop")continue;
            auto surface = panels[i].session->active() ? panels[i].session->window() : Unbound::panelSurface(page(),panels[i].name);
            if(titleDrags)titleDrags->wire(surface,panels[i].name,panels[i].label,[this,i](QPoint){beginPanelDrag(i);});
            if (panels[i].name=="frameConformTimelines") {
                timelineControl->wire(surface,[this,i] { togglePanelPlacement(i); },[this,i](QPoint) {beginPanelDrag(i);});
                continue;
            }
            nativeDecoration->wire(surface,panels[i].name,panels[i].label,panels[i].session->floating(),
                [this,i] { togglePanelPlacement(i); },
                [this,i](QPoint) {beginPanelDrag(i);});
        }
        if (workspaceMenu) workspaceMenu->wire();
        if (toolbarDoors) toolbarDoors->wire(Unbound::panelSurface(page(),"unboundWorkspaceToolbar"));
        if(titleDrags)for(int i=0;i<panels.size();++i)if(panels[i].name!="unboundWorkspaceToolbar")
            titleDrags->wire(Unbound::panelSurface(page(),"unboundWorkspaceToolbar"),panels[i].name,panels[i].label,[this,i](QPoint){beginPanelDrag(i);});
        if (viewerMode && page()) viewerMode->refresh(page()->window());
        // Timeline toolbar stays with Timeline; no independent detach control.
        QWidget *root = page();
        if(root&&root->isVisible()&&!pendingMainPlacement.isEmpty()) {
            const auto placement=pendingMainPlacement;pendingMainPlacement={};
            auto main=root->window();if(main->isMaximized())main->showNormal();
            const auto frame=Unbound::WorkspaceSettings::rectangle(placement);if(frame.isValid())main->setGeometry(frame);
            if(placement["maximized"].toBool())main->showMaximized();
            if(mainPlacementRecord.contains("display")&&Unbound::DisplayPlacement::match(mainPlacementRecord,Unbound::workspaceDisplays())<0)
                mainPlacementRecord["display_fallback"]=Unbound::DisplayPlacement::rect(main->isMaximized()?main->normalGeometry():main->geometry());
        }
        updateNativeSizing();
        if (!root || !root->isVisible()) return;
        for(const auto &panel:panels)if(!panel.session->active() && !panel.desired && panel.saved.contains("native_hidden"))
            nativeSurfaceDoors.set(panel.name,Unbound::panelSurface(root,panel.name),!panel.saved["native_hidden"].toBool());
        QList<Unbound::WorkspaceRestoration::Panel> restoring;
        for (const auto &panel:panels) restoring.append({panel.name,panel.saved,panel.desired,panel.session});
        const auto result=workspaceRestore->native(root,nativeWorkspace,restoring,[this](int i) { requestUndock(i,true); });
        if (!result.error.isEmpty()) {status->setText(result.error);workspaceRestore->failPreset(result.error);}
        if (!result.prepared) return;
        workspaceRestore->finishPreset();
        workspaceRestore->settled(!pendingGroups.isEmpty(),background->isChecked(),
            [this] { restoreGroups(); },[this] { if (background->isChecked()) applyBackgroundVisibility(true); });
    }
    void keepGripsReachable()
    {
        Unbound::keepOnDisplay(this);
        for (auto &p : panels) if (p.session->active()) p.session->bringOnScreen();
    }
    void rescue()
    {
        Unbound::logEvent(QStringLiteral("workspace.recover")); Unbound::keepOnDisplay(this);
        for (auto &p : panels) if (p.session->active()) p.session->centerOnCurrentDisplay();
    }
    Unbound::WorkspacePersistence::Sources persistenceSources()
    {
        Unbound::WorkspacePersistence::Sources source;
        for (auto &panel:panels) source.panels.append({panel.name,panel.desired,panel.session,&panel.saved});
        source.groups=groups; source.pendingGroups=pendingGroups;
        source.nativeWorkspace=nativeWorkspace; source.nativeDoors=&nativeSurfaceDoors;
        source.expansion=expansionController; source.footer=footer; source.header=workspaceHeader;
        source.main=Unbound::resolveMainWindow(); source.mainPlacement=&mainPlacementRecord;
        return source;
    }
    void save(bool retainSelection=false)
    {
        if (quitting) return;
        Unbound::WorkspaceSnapshot options;
        options.pinControls=pin->isChecked(); options.hideBackground=background->isChecked();
        options.selected=presets->currentData().toString(); options.chrome=style->currentIndex();
        options.panelHierarchy=hierarchyOption->isChecked(); options.snapWindows=snap->isChecked();
        options.keepOnScreen=constrain->isChecked(); options.controls=geometry();
        const auto result=persistence.save({quitting,workspaceRestore && workspaceRestore->savingBlocked(),
            restoringNative,retainSelection,layoutMode},persistenceSources(),options);
        if (!result.performed) return;
        if (!result.success) status->setText(QStringLiteral("Could not save the workspace. Check the diagnostic log."));
        QElapsedTimer phases;phases.start();
        updatePresetState();
        const auto menuMs=phases.nsecsElapsed()/1000000.;phases.restart();
        if (headers) headers->schedule();
        if(workspaceHeader)workspaceHeader->schedule();
        if (toolbarDoors) toolbarDoors->wire(Unbound::panelSurface(page(),"unboundWorkspaceToolbar"));
        if(titleDrags)for(int i=0;i<panels.size();++i)if(panels[i].name!="unboundWorkspaceToolbar")
            titleDrags->wire(Unbound::panelSurface(page(),"unboundWorkspaceToolbar"),panels[i].name,panels[i].label,[this,i](QPoint){beginPanelDrag(i);});
        Unbound::logEvent("workspace.save.phases",{{"snapshot_ms",result.snapshotMs},{"persist_ms",result.persistMs},
            {"menu_ms",menuMs},{"header_ms",phases.nsecsElapsed()/1000000.}});
    }
    void updateNativeSizing()
    {
        if (quitting || restoringNative || expansionController->index()>=0 || !nativeSizing ||
            (nativeWorkspace && !nativeWorkspace->requiredPanels().isEmpty())) return;
        auto root=page();
        QList<Unbound::NativeWorkspacePresentation::Surface> surfaces;
        for (const auto &panel:panels) surfaces.append({panel.name,Unbound::panelSurface(root,panel.name),panel.session,panel.label.endsWith("Toolbar")});
        nativePresentation.reconcile(Unbound::resolveMainWindow(),root,nativeSizing,nativeWorkspace && nativeWorkspace->active(),surfaces,background->isChecked(),layoutMode=="managed");
    }
    void shutdown()
    {
        if (quitting) return;
        restoreExpanded(); save(); quitting = true; saveTimer.stop(); restoreTimer.stop();
        if (workspaceRestore) workspaceRestore->cancel();
        if (workspaceHeader) workspaceHeader->shutdown();
        if (footer) footer->shutdown();
        groupReserve->clear();
        if (nativeWorkspace) nativeWorkspace->release(true);
        if (nativeSizing) nativeSizing->restore();
        if (auto root = page()) if (root->window()->property("unboundBackgroundHidden").toBool()) Unbound::setNativeBackgroundHidden(root->window(), {}, false);
        for (auto &p : panels) if (p.session->active()) p.session->dock();
    }
    Unbound::EditPageLifecycle *pageLifecycle=nullptr;
    Unbound::ToolbarDoors *toolbarDoors=nullptr;
    Unbound::WorkspaceHeaders *headers=nullptr;
    Unbound::PanelManagerView *managerView=nullptr;
    Unbound::ControlScroll *optionsScroll = nullptr;
    QLabel *status = nullptr;
    QCheckBox *pin = nullptr, *constrain = nullptr;
    QCheckBox *snap = nullptr;
    QCheckBox *background = nullptr;
    QComboBox *style = nullptr;
    QCheckBox *hierarchyOption = nullptr;
    Unbound::WindowHierarchy *hierarchy = nullptr;
    Unbound::GroupHostReserve *groupReserve = nullptr;
    Unbound::NativeWorkspaceSizing *nativeSizing = nullptr;
    Unbound::NativeWorkspacePresentation nativePresentation;
    Unbound::NativeWorkspaceBinding *nativeWorkspace = nullptr;
    Unbound::NativePanelTitleDrags *titleDrags=nullptr;
    Unbound::PanelMenus *nativeMenus = nullptr;
    QList<Panel> panels;
    Unbound::NativeSurfaceDoors nativeSurfaceDoors;
    Unbound::NativePanelToggles *nativeToggles=nullptr;
    Unbound::NativePanelNavigation *nativeNavigation=nullptr;
    QSet<int> pendingNativeDoors,pendingPrimaryPanels;
    Unbound::WorkspaceSettings settings;
    Unbound::WorkspacePersistence persistence{settings};
    Unbound::WorkspaceRestoration *workspaceRestore=nullptr;
    Unbound::ArrangementControls *presetsUi = nullptr;
    Unbound::PanelCommandCatalog *commands=nullptr;
    Unbound::WorkspaceArrangements *workspaceMenu = nullptr;
    QComboBox *presets = nullptr;
    Unbound::PanelExpansionController *expansionController=nullptr;
    Unbound::ViewerModeCommand *viewerMode = nullptr;
    Unbound::ToolbarDecoration *toolbarDecoration = nullptr;
    Unbound::NativePanelDecoration *nativeDecoration = nullptr;
    Unbound::TimelineControl *timelineControl = nullptr;
    Unbound::NativePanelDrag *nativeDrag = nullptr;
    Unbound::WorkspaceCallers *callers = nullptr;
    Unbound::FooterPanel *footer = nullptr;
    Unbound::PinnedWorkspaceHeader *workspaceHeader=nullptr;
    mutable QPointer<QWidget> editPageCache;
    mutable QElapsedTimer pageScanClock;
    QTimer saveTimer, restoreTimer;
    QString layoutMode="native";
    QJsonObject pendingMainPlacement,mainPlacementRecord;
    bool quitting = false,restoringNative=false;
    bool toggleScanQueued = false;
    unsigned bulkVersion = 0;
    QList<Unbound::PanelGroup *> groups;
    QJsonArray pendingGroups;
    unsigned groupVersion = 0;
};
