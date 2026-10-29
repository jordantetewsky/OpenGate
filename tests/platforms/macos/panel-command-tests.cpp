#include "../../core/workspace-callers/fixtureevidence.h"
// SPDX-License-Identifier: MIT
#include <QTemporaryDir>
#include <QLineEdit>
#include <cstdio>
#include "panelcommandcatalog.h"
#include "nativepaneltoggles.h"
#include "nativepanelvisibility.h"
#include "workspacesnapshot.h"
#include "panelexpansioncontroller.h"
#include <QPushButton>
int main(int argc,char **argv) {
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1");
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files(OpenGateTest::evidenceTemplate("panel-command-tests"));OpenGateTest::retainEvidence(files); qputenv("UNBOUND_SHORTCUTS_FILE",(files.path()+"/Shortcuts.json").toUtf8());
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen); main.show(); QApplication::setActiveWindow(&main);
    bool ready=true,editVisible=true; int source=0,program=0,navigation=0;
    Unbound::PanelCommands commands(nullptr); commands.context=[&] { return ready; };
    commands.add({"source","Show / Hide Source",[&] { return editVisible; },[&] { ++source; }});
    commands.add({"program","Show / Hide Program",[&] { return editVisible; },[&] { ++program; }});
    commands.add({"navigation","Show / Hide Page Navigation",[] { return true; },[&] { ++navigation; }});
    check(commands.save({{"source","F17"},{"program","F18"},{"navigation","F20"}}).isEmpty(),"save independent bindings");
    const auto send=[&](int key,bool repeat=false) {
        QKeyEvent over(QEvent::ShortcutOverride,key,Qt::NoModifier); QApplication::sendEvent(&main,&over);
        QKeyEvent press(QEvent::KeyPress,key,Qt::NoModifier,QString(),repeat); QApplication::sendEvent(&main,&press);
        QApplication::processEvents();
    };
    QApplication::processEvents(); send(Qt::Key_F17); check(source==1 && program==0,"Source command never invokes Program");
    send(Qt::Key_F18); check(source==1 && program==1,"Program command never invokes Source");
    send(Qt::Key_F18,true); check(program==1,"auto-repeat does not toggle doors repeatedly");
    QLineEdit edit(&main); edit.show(); QApplication::setActiveWindow(&main); edit.setFocus(); QApplication::processEvents();
    check(QApplication::focusWidget()==&edit,"typing fixture establishes actual text focus"); send(Qt::Key_F17); check(source==1,"typing yields to native text input");
    edit.clearFocus(); ready=false; send(Qt::Key_F17); check(source==1,"inactive page cannot dispatch a panel command"); ready=true;
    check(!commands.save({{"source","F17"},{"program","F17"}}).isEmpty(),"duplicate bindings rejected atomically");
    Unbound::PanelCommands reopened(nullptr); check(reopened.assigned().value("program")=="F18","bindings survive reopening after failed edit");
    QAction native(&main); native.setShortcut(Qt::Key_F19); main.addAction(&native);
    check(!commands.save({{"source","F19"}}).isEmpty(),"native shortcut conflicts rejected");
    native.setShortcut(Qt::Key_F17); QApplication::processEvents();
    send(Qt::Key_F17); check(source==1,"a newly assigned native collision blocks only that Unbound binding");
    send(Qt::Key_F18); check(program==2,"one native collision cannot disable unrelated panel commands");
    native.setShortcut(Qt::Key_F19); QApplication::processEvents();
    send(Qt::Key_F17); check(source==2,"removing a native collision restores the saved Unbound binding");
    editVisible=false; send(Qt::Key_F17); send(Qt::Key_F20);
    check(source==2 && navigation==1,"global Page Navigation remains callable while Edit panel commands are unavailable");
    auto dialog=Unbound::createPanelShortcutEditor(&commands,&main); dialog->setAttribute(Qt::WA_DontShowOnScreen);
    check(dialog->findChild<QKeySequenceEdit *>("binding_source")!=nullptr,"every command has a discoverable editable binding"); delete dialog;
    {
        qputenv("UNBOUND_SHORTCUTS_FILE",(files.path()+"/Hover.json").toUtf8());
        Unbound::PanelCommands hover(nullptr); hover.context=[] { return true; }; int expansions=0;
        hover.add({"panel.expand.hovered","Maximize / Restore Panel Under Pointer",[] { return true; },[&] { ++expansions; }});
        hover.defaultBinding("panel.expand.hovered","`");
        QShortcut disabledNative(QKeySequence("`"),&main); disabledNative.setEnabled(false);
        check(hover.save(hover.assigned()).isEmpty(),"hovered-panel backtick is an editable default rather than an unassignable reservation");
        QApplication::processEvents(); send(Qt::Key_QuoteLeft);
        check(expansions==1,"disabled native backtick shortcut does not block maximize dispatch");
        disabledNative.setEnabled(true);
        check(!hover.validate(hover.assigned()).isEmpty(),"enabled native backtick shortcut still conflicts");
        disabledNative.setEnabled(false); expansions=0;
        check(hover.save({{"panel.expand.hovered","F24"}}).isEmpty(),"hovered panel maximize can be reassigned");
        QApplication::setActiveWindow(&main); edit.clearFocus(); QApplication::processEvents();
        check(hover.invoke("panel.expand.hovered","native-workflow-fixture") && expansions==1,"native workflow invokes the same registered expansion route");
        check(!hover.invoke("unknown","native-workflow-fixture") && expansions==1,"native workflow cannot invoke arbitrary IDs");
    }
    {
        qputenv("UNBOUND_SHORTCUTS_FILE",(files.path()+"/Catalog.json").toUtf8());
        // This block verifies the explicit legacy recovery provider. Ordinary
        // launch and live native remapping are covered by native-preset-bindings.
        const auto previousProvider=qgetenv("OPENGATE_NATIVE_PRESET_SHORTCUTS");
        qputenv("OPENGATE_NATIVE_PRESET_SHORTCUTS","0");
        Unbound::PanelCommandCatalog catalog(&main,[&] { return ready; });
        if(previousProvider.isNull())qunsetenv("OPENGATE_NATIVE_PRESET_SHORTCUTS");
        else qputenv("OPENGATE_NATIVE_PRESET_SHORTCUTS",previousProvider);
        check(!catalog.nativeMatch,"legacy fixture explicitly selects recovery provider");
        int calls=0,expansions=0,footerCalls=0;
        catalog.addPanel("source","Source Monitor",[&] { return editVisible; },[&] { ++calls; },[&] { ++expansions; });
        catalog.addNavigation([] { return true; },[&] { ++footerCalls; return QString(); });
        check(catalog.save({{"panel.toggle.source","F21"},{"panel.expand.source","F22"},
            {"panel.toggle.page-navigation","F23"}}).isEmpty(),"catalog retains stable command IDs");
        QApplication::setActiveWindow(&main); QApplication::processEvents();
        send(Qt::Key_F21); send(Qt::Key_F23);
        check(calls==0 && footerCalls==1,"catalog applies panel availability independently of global navigation");
        editVisible=true; send(Qt::Key_F21); send(Qt::Key_F22);
        check(calls==1 && expansions==1,"catalog dispatches separate door and expansion commands");
        ready=false; send(Qt::Key_F23); check(footerCalls==1,"shutdown context blocks catalog dispatch"); ready=true;
    }
    {
        QWidget nativeRoot; nativeRoot.setAttribute(Qt::WA_DontShowOnScreen);nativeRoot.show(); bool active=true,hidden=false; int requested=0,nativeSignals=0;
        Unbound::NativePanelToggles bridge(nullptr);
        bridge.add("Inspector",[&] { return active; },[&] { return !hidden; },[&] { ++requested; });
        auto button=new QPushButton(&nativeRoot); button->setCheckable(true);
        button->setObjectName("btnEditToggleInspectorInHeader");
        QObject::connect(button,&QAbstractButton::toggled,[&] { ++nativeSignals; });
        button->show();bridge.wire(&nativeRoot); bridge.wire(&nativeRoot); bridge.sync(&nativeRoot);
        check(button->isChecked() && nativeSignals==0 && requested==0,"sync blocks native signals and does not invoke a door");
        button->setChecked(false); check(requested==0,"programmatic native checked changes are observations, not panel commands");
        button->click(); check(requested==1,"one explicit activation dispatches one panel command after repeated wiring");
        active=false; hidden=false; bridge.sync(&nativeRoot);
        check(button->isChecked(),"inactive panel sync leaves native state alone");
        delete button;
        button=new QPushButton(&nativeRoot); button->setCheckable(true);
        button->setObjectName("btnEditToggleInspectorInHeader");
        active=true;button->show();bridge.wire(&nativeRoot); button->click();
        check(requested==2,"replacement native buttons are wired after old buttons are destroyed");
    }
    {
        QWidget root;root.setAttribute(Qt::WA_DontShowOnScreen);root.show();
        QPushButton pool(&root),index(&root);pool.setObjectName("btnEditToggleMediaPoolInSSM");index.setObjectName("btnEditToggleEditIndexInSSM");
        pool.setCheckable(true);index.setCheckable(true);pool.show();index.show();
        QButtonGroup radio;radio.addButton(&pool);radio.addButton(&index);radio.setExclusive(true);
        bool poolShown=true,indexShown=false;int requests=0;
        Unbound::NativePanelToggles bridge(nullptr);
        bridge.add("MediaPool",[]{return true;},[&]{return poolShown;},[&]{++requests;});
        bridge.add("EditIndex",[]{return true;},[&]{return indexShown;},[&]{++requests;});
        bridge.wire(&root);bridge.sync(&root);
        index.setChecked(true);QApplication::processEvents();
        check(requests==0 && pool.isChecked()&&!index.isChecked(),"native radio peer notifications cannot select an inactive OpenGate tab");
        poolShown=false;bridge.sync(&root);
        check(!pool.isChecked()&&!index.isChecked()&&radio.exclusive(),"all hidden panels clear radio checks while preserving native group policy");
        poolShown=true;indexShown=true;bridge.sync(&root);
        check(pool.isChecked()&&index.isChecked(),"separately visible OpenGate panels can both be checked despite native exclusivity");
        Unbound::setNativePanelVisibility(&root,"MediaPool",false);
        check(requests==0,"explicit native acquisition never queues OpenGate recall");
    }
    {
        const QJsonObject previous{{"hidden",true},{"pinned",true},{"width",321},{"custom","retained"}};
        const auto inactive=Unbound::WorkspaceSnapshot::panel(previous,false,false,{},false,false);
        check(inactive["hidden"].toBool() && inactive["width"].toInt()==321 && inactive["custom"]=="retained" &&
            !inactive["floating"].toBool(),"inactive snapshots preserve saved geometry and hidden/pin state");
        const auto active=Unbound::WorkspaceSnapshot::panel(previous,true,true,QRect(10,20,400,300),false,true);
        check(Unbound::WorkspaceSettings::rectangle(active)==QRect(10,20,400,300) && !active.contains("custom") &&
            !active["hidden"].toBool() && active["pinned"].toBool(),"active snapshots capture actual frame and door/pin state");
        Unbound::WorkspaceSnapshot snapshot; snapshot.panels={{"source",active}}; snapshot.selected="saved-but-modified";
        snapshot.hideBackground=true; snapshot.footer={{"floating",true},{"hidden",true},{"pinned",true}};
        const auto serialized=snapshot.json();
        check(serialized["panels"].toObject()["source"]==active && serialized["footer"].toObject()==snapshot.footer &&
            serialized["selected_arrangement"]==snapshot.selected && serialized["native_workspace_mode"]=="floating",
            "last-view schema retains independent doors, footer state and selected preset identity");
    }
    {
        Unbound::ManagedPanelSession session;
        Unbound::PanelExpansionController expansion(nullptr);
        int undocked=0,cancelled=0,restored=0,nativeRestored=0,saved=0;
        expansion.running=[] { return true; }; expansion.update=[](int) {}; expansion.save=[&] { ++saved; };
        expansion.report=[&](QString) { check(false,"pending expansion restore has no error"); };
        Unbound::PanelExpansionController::Target target;
        target.index=4; target.name="source"; target.session=&session;
        target.saved={{"width",420},{"height",240},{"hidden",true}}; target.native={{"original",true}};
        target.pending=[] { return true; }; target.undock=[&] { ++undocked; };
        target.cancelPending=[&] { ++cancelled; }; target.restored=[&] { ++restored; };
        target.restoreSaved=[&](QJsonObject value) { check(value["hidden"].toBool(),"expansion restores original hidden state"); };
        target.restoreNative=[&](QJsonObject value) { if (value["original"].toBool()) ++nativeRestored; return QString(); };
        expansion.begin(target,&main);
        check(expansion.index()==4 && undocked==1 && expansion.panelSnapshot(4,{})==target.saved &&
            expansion.nativeSnapshot({})==target.native,"pending expansion preserves original persistence snapshots");
        expansion.restore(); QApplication::processEvents();
        check(expansion.index()==-1 && cancelled==1 && restored==1 && nativeRestored==1 && saved==1,
            "restoring before readiness cancels pending work and restores exactly once");
        expansion.restore(); QApplication::processEvents();
        check(cancelled==1 && saved==1,"stale readiness and repeated restore cannot replay a completed expansion");
        int readinessErrors=0; expansion.report=[&](QString error) { if (!error.isEmpty()) ++readinessErrors; };
        expansion.begin(target,nullptr); QApplication::processEvents();
        check(expansion.index()==-1 && cancelled==2 && restored==2 && nativeRestored==2 && saved==2 && readinessErrors==1,
            "unavailable expansion surface rolls back original presentation instead of saving temporary state");
    }
    {
        QWidget root,page(&root),floating(&root,Qt::Tool);auto toggle=new QPushButton(&floating);
        toggle->setObjectName("btnEditToggleMixerInSSM");toggle->setCheckable(true);toggle->setChecked(true);
        Unbound::NativePanelToggles bridge(nullptr);int calls=0;
        bridge.add("Mixer",[] { return true; },[] { return false; },[&] { ++calls; });
        root.setAttribute(Qt::WA_DontShowOnScreen);root.show();floating.show();toggle->show();bridge.wire(&page);bridge.sync(&page);
        check(!toggle->isChecked() && calls==0,"native toggle sync reaches a floated Workspace Toolbar without emitting native signals");
        toggle->click();check(calls>0,"floated native toolbar retains its panel-door bridge");
    }
    Unbound::flushDiagnostics(); QFile events(files.path()+"/events.jsonl"); events.open(QIODevice::ReadOnly);
    check(events.readAll().contains("command.invoke"),"command origin and acceptance logged");
    if (!failures) std::puts("PASS: independent commands, persistence, conflicts, typing, page scope, repeats and editor discovery");
    return failures ? 1 : 0;
}
