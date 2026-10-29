// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "workspacearrangements.h"
#include "panelcallermenu.h"
#include "nativepaneldecoration.h"
#include "arrangementcontrols.h"
#include "pagecontext.h"
#include "vieweractionregistry.h"
#include "panelcommandmenu.h"
int main(int argc, char **argv)
{
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");
    const auto foreground = Unbound::nativeFrontmostProcessForTest();
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir temporary;
    qputenv("UNBOUND_ARRANGEMENTS_FILE", (temporary.path()+"/Arrangements.json").toUtf8());
    qputenv("UNBOUND_DIAGNOSTICS_DIR", temporary.path().toUtf8());
    int failures = 0;
    const auto check = [&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    {
        QWidget main; QObject owner; QMenu menu;
        Unbound::ViewerActionRegistry registry(&owner);
        check(registry.actions(&main).isEmpty(), "viewer registry starts without invented commands");
        auto action = new QAction("Single Viewer Mode", &menu); menu.addAction(action);
        check(registry.actions(&main) == QList<QAction *>{action}, "late native menu command invalidates discovery cache");
        action->setShortcut(QKeySequence("Shift+W")); action->setChecked(true);
        check(registry.actions(&main).first()->shortcut() == QKeySequence("Shift+W"), "cached command retains live shortcut identity");
        menu.removeAction(action); delete action;
        check(registry.actions(&main).isEmpty(), "destroyed native commands cannot leave stale pointers");
        auto replacement = menu.addAction("Single Viewer Mode");
        check(registry.actions(&main) == QList<QAction *>{replacement}, "rebuilt menu registers its replacement command");
    }
    {
        QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen);
        auto pages = new QStackedWidget(&main); (new QVBoxLayout(&main))->addWidget(pages);
        auto edit = new QWidget(pages); edit->setObjectName("WStackPage_Conform"); pages->addWidget(edit);
        auto color = new QWidget(pages); color->setObjectName("WStackPage_Color"); pages->addWidget(color);
        auto audio = new QWidget(pages); audio->setObjectName("WStackPage_Audio"); pages->addWidget(audio);
        main.show();
        check(Unbound::currentWorkspacePage(&main) == "edit","current page follows native Edit widget visibility");
        pages->setCurrentWidget(color);
        check(Unbound::currentWorkspacePage(&main) == "color","page identity follows the active native stack without a key mapping");
        pages->setCurrentWidget(audio);
        check(Unbound::currentWorkspacePage(&main) == "fairlight","native Audio page maps to the Fairlight menu label");
        main.hide();
        check(Unbound::currentWorkspacePage(&main).isEmpty(),"hidden or unavailable pages are not guessed to be Edit");
    }
    check(Unbound::productName()=="OpenGate" && Unbound::productVendor()=="CineWaltz" &&
        Unbound::productCredit().contains("Unbound"),"OpenGate identity preserves explicit foundation credit");
    Unbound::ArrangementStore store; QString first,second;
    check(store.create("Edit & Review",{{"version",1}},first).isEmpty() && store.create("Audio",{{"version",1}},second).isEmpty(), "create stable saved workspace identities");
    {
        QWidget nativePanel; auto row = new QHBoxLayout(&nativePanel);
        auto options = new QToolButton(&nativePanel); options->setObjectName("btnMediaPoolOptions"); row->addWidget(options);
        Unbound::NativePanelDecoration decoration(&nativePanel); int toggles=0;
        decoration.wire(&nativePanel,"pool","Media Pool",false,[&] { ++toggles; });
        decoration.wire(&nativePanel,"pool","Media Pool",true,[&] { ++toggles; });
        auto button = nativePanel.findChild<QToolButton *>("unboundNativeUndock_pool");
        check(button && row->indexOf(button)+1 == row->indexOf(options) &&
            !nativePanel.findChild<QToolButton *>("unboundNativeKeyboard_pool"),
            "one placement control occupies the native slot immediately before options");
        check(nativePanel.findChildren<QToolButton *>("unboundNativeUndock_pool").size() == 1,"native header rewiring cannot duplicate the button");
        if (button) button->click();
        check(toggles == 1 && button->toolTip() == "Attach Media Pool","native header button dispatches once and reflects docking state");
        int drags=0;
        decoration.wire(&nativePanel,"pool","Media Pool",true,[&] { ++toggles; },[&](QPoint) { ++drags; });
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(4,4),QPointF(100,100),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        const int distance=QApplication::startDragDistance()+10;
        QMouseEvent move(QEvent::MouseMove,QPointF(4+distance,4),QPointF(100+distance,100),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(4,4),QPointF(100,100),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(button,&press); QApplication::sendEvent(button,&move); QApplication::sendEvent(button,&release);
        check(drags==1 && toggles==1,"native undock drag crosses threshold once and cannot become a dock click on release");
        QWidget floatingGroup;floatingGroup.setObjectName("unboundFloating_group_timeline_inspector");
        nativePanel.setParent(&floatingGroup);decoration.wire(&nativePanel,"pool","Media Pool",false,[&]{++toggles;});
        check(button&&!button->isHidden(),"a native panel nested inside a floating group keeps its own placement control");
        nativePanel.setParent(nullptr);
    }
    {
        QMenu nativeCommands; int opened=0;
        auto keyboard = nativeCommands.addAction(QString::fromUtf8("Keyboard Customization…"));
        QObject::connect(keyboard,&QAction::triggered,[&] { ++opened; });
        check(Unbound::openNativeKeyboardCustomization() && opened==1,"keyboard launcher invokes the native action once");
        keyboard->setEnabled(false);
        check(!Unbound::openNativeKeyboardCustomization() && opened==1,"disabled native keyboard command is not invoked");
        keyboard->setEnabled(true);
        QMenu ambiguous; ambiguous.addAction("Keyboard Customization...");
        check(!Unbound::openNativeKeyboardCustomization() && opened==1,"ambiguous native keyboard actions are rejected");
    }
    {
        QWidget panel; auto row=new QHBoxLayout(&panel); auto options=new QToolButton(&panel);
        options->setObjectName("btnMediaPoolOptions"); row->addWidget(options);
        { Unbound::NativePanelDecoration decoration(nullptr); decoration.wire(&panel,"lifetime","Media Pool",false,[] {}); }
        check(panel.findChildren<QToolButton *>().size()==1 && options->parentWidget()==&panel,
            "destroying header decoration removes owned buttons and stale drag callbacks while preserving native options");
    }
    {
        Unbound::PanelCallerMenu callers; QMenu root; int pageQueries=0;
        callers.currentPage=[&] { ++pageQueries; return QStringLiteral("edit"); };
        for(int i=0;i<20;++i) callers.add({QString::number(i),QString::number(i),"edit",[] {return true;},[] {return true;},[] {}});
        callers.wire(&root);
        check(pageQueries==1,"one panel menu refresh resolves page context once for the entire inventory");
        pageQueries=0; QMenu flat; callers.populateFlat(&flat);
        check(pageQueries==1 && flat.actions().size()==20,"flat inventory shares one current-page snapshot without omitting panels");
    }
    QMenuBar bar; bar.setAttribute(Qt::WA_DontShowOnScreen);
    QMenu workspace(&bar); workspace.setObjectName("workspace"); bar.addMenu(&workspace); bar.addMenu("Help");
    QMenu *root = nullptr;
    int managerCalls = 0;
    int nativeCalls = 0, restores = 0; QString selected = first, activePage = "edit", nativePage;
    const auto configure = [&](Unbound::WorkspaceArrangements &menus) {
        menus.entries = [&] { return store.list(); }; menus.selected = [&] { return selected; };
        menus.currentPage = [&] { return activePage; };
        menus.nativeWorkspace = [&](QString page) { ++nativeCalls; nativePage = page; };
        menus.invoke = [&](QString operation,QString id) {
            if (operation == "Restore" && !store.find(id).isEmpty()) { selected = id; ++restores; }
            if (operation == "Delete") { store.remove(id); if (id == selected) selected.clear(); }
            if (operation == "Update") store.update(id,{{"version",2}});
        };
    };
    {
        Unbound::WorkspaceArrangements menus(nullptr); configure(menus); menus.wire(); menus.wire();
        root = bar.findChild<QMenu *>("unboundWorkspaceUI");
        check(root && bar.actions().indexOf(root->menuAction()) == bar.actions().indexOf(workspace.menuAction())+1,
            "Workspace UI is a separate top-level menu beside Workspace");
        if (!root) return 1;
        check(root->title()=="OpenGate UI","top-level product menu is named OpenGate UI");
        Unbound::PanelCallerMenu callers; int panelShows=0; bool visible=false;
        callers.currentPage=[&] { return activePage; };
        callers.add({"source","Source Monitor","edit",[] { return true; },[&] { return visible; },[&] { visible=true; ++panelShows; },[&](bool value){visible=value;++panelShows;}});
        menus.panelMenu=[&](QMenu *menu) { return callers.wire(menu); }; menus.wire(); menus.wire();
        auto show=root->findChild<QMenu *>("openGateShowPanel");
        auto source=show ? show->findChild<QAction *>("openGateShowPanel_source",Qt::FindDirectChildrenOnly) : nullptr;
        check(show && show->title()=="Panels" && show->findChildren<QMenu *>().isEmpty() && source && source->isEnabled(),"Panels directly exposes current-page controls without category submenus");
        if (source) { source->trigger(); QApplication::processEvents(); menus.wire(); source->trigger(); QApplication::processEvents(); menus.wire(); }
        check(panelShows==2 && !visible && source && !source->isChecked(),"both entry points use the same Show/Hide state without placement changes");
        activePage="color"; menus.wire();
        check(source && !source->isEnabled(),"Edit-only callers are disabled on other pages");
        activePage="edit"; menus.wire(); if (source) source->trigger(); activePage="color"; QApplication::processEvents();
        check(panelShows==2,"queued panel request is cancelled after page change");
        activePage="edit"; menus.wire();
        auto submenu = root->findChild<QMenu *>("unboundWorkspaceArrangements");
        auto preset = root->findChild<QMenu *>("unboundWorkspace_"+second);
        check(submenu && submenu->title() == "Manage Saved Workspaces" && preset && !root->isVisible(), "menu is populated before native display");
        auto about=root->findChild<QAction *>("openGateAbout");
        check(about && about->text().contains("OpenGate"),"product attribution is discoverable from Workspace UI");
        auto manager = root->findChild<QAction *>("unboundShowPanelManager");
        check(manager && !manager->isEnabled(), "manager action is disabled without an owner callback");
        menus.showPanelManager = [&] { ++managerCalls; };
        menus.wire();
        manager->trigger(); QApplication::processEvents();
        check(managerCalls == 1, "Workspace UI opens the panel manager through its owner callback");
        int shortcutsCalls=0;
        menus.showPanelShortcuts=[&] { ++shortcutsCalls; }; menus.wire(); menus.wire();
        auto panelShortcuts=root->findChild<QAction *>("unboundPanelShortcuts");
        check(panelShortcuts && panelShortcuts->isEnabled() && panelShortcuts->text()==QString::fromUtf8("OpenGate Panel Shortcuts…"),"single shortcut entry retains its familiar label");
        check(!root->findChild<QAction *>("unboundKeyboardCustomization") && !root->findChild<QMenu *>("openGateNativeCommands"),"no duplicate keyboard entry or Commands submenu");
        if (panelShortcuts) panelShortcuts->trigger(); QApplication::processEvents();
        check(shortcutsCalls==1,"shortcut editor menu dispatches once after repeated synchronization");
        Unbound::PanelCommands commandRegistry(nullptr);
        commandRegistry.add({"edit.inspector.stabilize","Edit Inspector / Stabilize",[]{return true;},[]{}});
        Unbound::PanelCommandMenu commandPresentation(nullptr,&commandRegistry);
        menus.commandsMenu=[&](QMenu *parent){return commandPresentation.menu(parent);};menus.wire();menus.wire();
        auto commands=root->findChild<QMenu *>("openGateCommands");
        check(commands&&root->actions().contains(commands->menuAction())&&commands->actions().contains(panelShortcuts),"command submenu includes existing shortcut editor action");
        check(!root->actions().contains(panelShortcuts)&&root->findChildren<QAction *>("unboundPanelShortcuts").size()==1,"shortcut editor has one presentation, without duplicate actions");
        panelShortcuts->trigger();QApplication::processEvents();check(shortcutsCalls==2,"nested shortcut editor action retains its original callback");

        check(root->actions().first()->text() == "OpenGate Workspaces" && !workspace.findChild<QAction *>("unboundRestoreNativeWorkspace"),
            "OpenGate Workspaces is first; original Resolve recovery remains separate from native Workspace commands");
        check(root->findChild<QMenu *>("unboundWorkspace_"+first)->title() == "Edit && Review", "preset names retain literal ampersands in native menus");
        auto original = root->findChild<QAction *>("unboundWorkspaceRestore_"+second);
        original->trigger(); QApplication::processEvents(); menus.wire();
        check(restores == 1 && selected == second && preset->menuAction()->isChecked(), "menu restores stable ID once and synchronizes selection");
        store.rename(second,"Mix"); store.reorder({second,first}); menus.wire();
        QList<QString> ids; for (auto action : submenu->actions()) if (action->menu()) ids.append(action->menu()->property("unboundArrangementId").toString());
        check(preset->title() == "Mix" && root->findChild<QAction *>("unboundWorkspaceRestore_"+second) == original && ids == QList<QString>{second,first}, "rename and reorder synchronize without rebuilding action identity");
        original->trigger(); QApplication::processEvents();
        check(restores == 2, "repeated menu synchronization creates no duplicate restore callbacks");
        preset->findChild<QAction *>("unboundUpdateArrangement")->trigger(); QApplication::processEvents();
        check(store.find(second)["workspace"].toObject()["version"] == 2, "menu updates the selected workspace record");
        preset->findChild<QAction *>("unboundArrangement_Delete")->trigger(); QApplication::processEvents();
        check(store.find(second).isEmpty() && root->findChild<QAction *>("unboundRestoreNativeWorkspace"), "deleting a preset cannot delete the native recovery action");
        root->findChild<QAction *>("unboundRestoreNativeWorkspace")->trigger(); QApplication::processEvents();
        check(nativeCalls == 1, "native recovery remains callable after deleting the selected preset");
        QString color; store.create("Color Review",{{"page","color"},{"version",1}},color);
        activePage = "color"; menus.wire();
        auto colorRecall = root->findChild<QAction *>("unboundWorkspaceRestore_"+color);
        check(root->actions().first()->text() == "OpenGate Workspaces" && colorRecall && root->findChild<QMenu *>("openGateWorkspaces")->actions().contains(colorRecall) &&
            !root->actions().contains(root->findChild<QAction *>("unboundWorkspaceRestore_"+first)),
            "page switching relabels native recovery and filters saved workspaces without mixing Edit and Color");
        check(!colorRecall->isEnabled(),"unimplemented page backends cannot restore Edit content under another page's name");
        root->findChild<QAction *>("unboundRestoreNativeWorkspace")->trigger(); QApplication::processEvents();
        check(nativeCalls == 2 && nativePage == "color","native recovery receives the page shown in its label");
        menus.supportedPage = [](QString) { return true; }; menus.wire();
        colorRecall->trigger(); activePage = "edit"; QApplication::processEvents();
        check(restores == 2,"queued page-specific commands cannot run after switching to another page");
        store.remove(color); menus.wire();
    }
    {
        Unbound::WorkspaceArrangements reopened(nullptr); configure(reopened); reopened.wire();
        root->findChild<QAction *>("unboundRestoreNativeWorkspace")->trigger(); QApplication::processEvents();
        check(nativeCalls == 3, "native recovery rebinds once after controller recreation");
    }
    Unbound::ArrangementStore reloaded;
    check(reloaded.list().size() == 1 && reloaded.list()[0].toObject()["id"] == first, "saved deletion and order survive store reload");
    QString third; check(reloaded.create("Third",{{"version",1}},third).isEmpty(), "prepare widget reorder fixture");
    {
        QWidget owner; owner.setAttribute(Qt::WA_DontShowOnScreen); auto layout = new QVBoxLayout(&owner);
        Unbound::ArrangementControls controls(&owner,layout,first);
        controls.workspace = [] { return QJsonObject{{"version",3}}; }; controls.report = [](QString) {};
        int updates = 0; controls.changed = [&] { ++updates; };
        controls.invoke("Reorder"); auto order = owner.findChild<Unbound::ArrangementOrder *>("arrangementOrder");
        check(order && !order->isHidden() && order->count() == 2, "Unbound widget exposes reorderable grip rows");
        check(order->model()->moveRows(QModelIndex(),0,1,QModelIndex(),2), "grip list model accepts a real row move");
        QApplication::processEvents();
        Unbound::ArrangementStore reordered;
        check(updates == 1 && reordered.list()[0].toObject()["id"] == third && controls.selection()->currentData().toString() == first,
            "widget row move persists order while retaining the selected workspace identity");
        check(!reordered.reorder({third,third}).isEmpty(), "invalid reorder cannot duplicate or erase saved workspaces");
    }
    store = Unbound::ArrangementStore();
    for (const auto &entry : store.list()) store.remove(entry.toObject()["id"].toString());
    {
        Unbound::WorkspaceArrangements empty(nullptr); configure(empty); empty.wire();
        auto submenu = root->findChild<QMenu *>("unboundWorkspaceArrangements"); int presets = 0, recovery = 0;
        for (auto action : submenu->actions()) if (action->menu()) ++presets;
        for (auto action : root->actions()) if (action->objectName() == "unboundRestoreNativeWorkspace") ++recovery;
        check(presets == 0 && recovery == 1 && root->findChild<QAction *>("unboundSaveWorkspaceAs")->isEnabled(),
            "deleting every preset retains one permanent recovery command and Save As");
    }
    check(Unbound::nativeFrontmostProcessForTest() == foreground, "hidden workspace-menu tests preserve foreground app");
    if (!failures) std::puts("PASS: native workspace preset sync, stable identities, persistent grip ordering and permanent native recovery");
    return failures ? 1 : 0;
}
