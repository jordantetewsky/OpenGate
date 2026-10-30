#include "../../core/workspace-callers/fixtureevidence.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QDataStream>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QLineEdit>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "panelcommandcatalog.h"
#include "nativeeditcommands.h"
#include "../../core/workspace-callers/nativepresetfixture.h"
int main(int argc,char **argv){
 QApplication app(argc,argv);QTemporaryDir dir(OpenGateTest::evidenceTemplate("native-preset-bindings-tests"));OpenGateTest::retainEvidence(dir);
 const auto presetDirectory=dir.path()+"/presets",scriptDirectory=dir.path()+"/scripts";
 QDir().mkpath(presetDirectory);QDir().mkpath(scriptDirectory);
 qputenv("UNBOUND_DIAGNOSTICS_DIR",(dir.path()+"/logs").toUtf8());
 qputenv("UNBOUND_SHORTCUTS_FILE",(dir.path()+"/Shortcuts.json").toUtf8());
 int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
 const QByteArray config="<Config><CustomKeyboardPreset>Active</CustomKeyboardPreset></Config>";
 const QString stabilize="OpenGate - Edit - Stabilize";
 const QKeySequence s("Alt+Shift+S"),t("Alt+Shift+T");
 auto xml=nativePresetFixture({{stabilize,{s,t}},{"Other",{QKeySequence("F18")}}});
 auto snapshot=Unbound::NativePresetBindings::decode(config,xml);
 check(snapshot.error.isEmpty() && snapshot.present && snapshot.commands.value(stabilize)==QList<QKeySequence>{s,t},"decode real Qt serialization and alternates");
 auto missing=Unbound::NativePresetBindings::decode(config,nativePresetFixture({},"Different"));
 check(missing.error.isEmpty()&&!missing.present&&missing.commands.isEmpty(),"absent active preset clears assignments");
 check(!Unbound::NativePresetBindings::decode(config,xml.left(xml.size()/2)).error.isEmpty(),"truncated XML fails closed");
 auto conflict=Unbound::NativePresetBindings::decode(config,nativePresetFixture({{stabilize,{s}},{"nativeConflict",{s}}}));
 check(conflict.owners.value(s.toString(QKeySequence::PortableText)).size()==2,"native conflicts retained");
 auto save=[&](const QString &name,const QByteArray &data){QSaveFile f((name.endsWith(".py")?scriptDirectory:presetDirectory)+'/'+name);f.open(QIODevice::WriteOnly);f.write(data);f.commit();};
 save("config.user.xml",config);save("keyboard.preset.xml",xml);
 qunsetenv("OPENGATE_NATIVE_PRESET_SHORTCUTS");qputenv("OPENGATE_PRESET_DIRECTORY",presetDirectory.toUtf8());
 QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);
 QWidget edit(&main);edit.setObjectName("WStackPage_Conform");edit.show();main.show();QApplication::setActiveWindow(&main);QApplication::processEvents();
 auto timeline=new QWidget(&edit);timeline->setObjectName("frameConformTimelines");timeline->setFocusPolicy(Qt::StrongFocus);timeline->show();timeline->setFocus();
 int calls=0;Unbound::PanelCommandCatalog commands(&main,[]{return true;});
 check(commands.nativeShortcutOwners(s).contains(stabilize),"navigation lookup reuses the current native preset snapshot");
 qputenv("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",scriptDirectory.toUtf8());
 save("OpenGate - Edit - Stabilize.py","# opengate-command-route: selected-timeline-api-v1\n");
 QMenu workflow(&main);auto nativeAction=workflow.addAction(stabilize);nativeAction->setObjectName("workspaceWIPlugins_fixture");
 QObject::connect(nativeAction,&QAction::triggered,[&] {++calls;});
 Unbound::NativeEditCommands editCommands(&main,[&] {return &edit;});editCommands.addTo(&commands);
 int expanded=0;commands.add({"panel.expand.hovered","Maximize / Restore Hovered Panel",[]{return true;},[&] {++expanded;}});
 commands.defaultBinding("panel.expand.hovered","`");
 auto press=[&](QWidget *receiver,int key,Qt::KeyboardModifiers mods,int expectedCalls,int expectedExpanded){QApplication::setActiveWindow(receiver->window());QKeyEvent over(QEvent::ShortcutOverride,key,mods);QApplication::sendEvent(receiver,&over);QKeyEvent down(QEvent::KeyPress,key,mods);QApplication::sendEvent(receiver,&down);std::fprintf(stderr,"KEY %d active=%s focus=%s popup=%d modal=%d calls=%d expanded=%d\n",key,qPrintable(QApplication::activeWindow()?QApplication::activeWindow()->objectName():QString("none")),qPrintable(QApplication::focusWidget()?QApplication::focusWidget()->metaObject()->className():QString("none")),bool(QApplication::activePopupWidget()),bool(QApplication::activeModalWidget()),calls,expanded);
  QElapsedTimer delivery;delivery.start();
  // Deliver asynchronous command callbacks before the assertion or next preset
  // rewrite. One processEvents call need not drain newly registered zero timers.
  do{QCoreApplication::sendPostedEvents(nullptr,QEvent::MetaCall);QApplication::processEvents();QThread::msleep(1);}while(delivery.elapsed()<30 || ((calls!=expectedCalls || expanded!=expectedExpanded)&&delivery.elapsed()<1000));
 };
 press(&main,Qt::Key_S,Qt::AltModifier|Qt::ShiftModifier,1,0);check(calls==1,"persisted assignment executes registry once");
 press(&main,Qt::Key_QuoteLeft,Qt::NoModifier,1,1);check(expanded==1,"legacy default remains only when native preset has no hovered command row");
 press(&main,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,2,1);check(calls==2,"alternate binding executes");
 QLineEdit typing(&edit);typing.show();typing.setFocus();QApplication::processEvents();
 press(&typing,Qt::Key_S,Qt::NoModifier,2,1);check(calls==2,"ordinary typing remains text entry");typing.clearFocus();timeline->setFocus();
 edit.hide();press(&main,Qt::Key_S,Qt::AltModifier|Qt::ShiftModifier,2,1);check(calls==2,"no execution outside Edit");edit.show();
 QWidget floating(&main,Qt::Window);floating.setObjectName("unboundFloating_fixture");floating.setAttribute(Qt::WA_DontShowOnScreen);floating.show();QApplication::setActiveWindow(&floating);QApplication::processEvents();timeline->setParent(&floating);timeline->show();timeline->setFocus();
 press(&floating,Qt::Key_S,Qt::AltModifier|Qt::ShiftModifier,3,1);check(calls==3,"same assignment works from floating surface");
 // Atomic rewrites invalidate old bindings and restore watches after replacement.
 Unbound::NativePresetBindings watched(nullptr,presetDirectory);
 auto settle=[] {QElapsedTimer timer;timer.start();while(timer.elapsed()<1500){QApplication::processEvents();QThread::msleep(1);}};
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}}}));settle();
 check(watched.snapshot().commands.value(stabilize)==QList<QKeySequence>{t},"atomic assignment replacement reloads");
 press(&floating,Qt::Key_S,Qt::AltModifier|Qt::ShiftModifier,3,1);check(calls==3,"removed assignment no longer reaches the native API route");
 press(&floating,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,4,1);check(calls==4,"remapped key reaches actual Stabilize workflow route without Inspector");
 save("config.user.xml","<Config><CustomKeyboardPreset>Different</CustomKeyboardPreset></Config>");settle();
 check(!watched.snapshot().present&&watched.snapshot().commands.isEmpty(),"preset switch clears prior assignment");
 press(&floating,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,4,1);check(calls==4,"switching preset cannot retain previous native command binding");
 save("config.user.xml",config);save("keyboard.preset.xml","broken");settle();
 check(!watched.snapshot().error.isEmpty()&&watched.snapshot().commands.isEmpty(),"bad rewrite cannot retain stale assignments");
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}}}));settle();
 check(watched.snapshot().error.isEmpty()&&watched.snapshot().commands.value(stabilize)==QList<QKeySequence>{t},"watch survives atomic replacements and parse recovery");
 press(&floating,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,5,1);check(calls==5,"restored preset dispatches native API route exactly once after recovery");
 const QString hovered="OpenGate - Maximize-Restore Hovered Panel";
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}},{hovered,{QKeySequence("F18")}}}));settle();
 press(&floating,Qt::Key_QuoteLeft,Qt::NoModifier,5,1);check(expanded==1,"native maximize assignment suppresses legacy backtick");
 press(&floating,Qt::Key_F18,Qt::NoModifier,5,2);check(expanded==2,"native maximize assignment reaches shared expansion command");
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}},{hovered,{}}}));settle();
 press(&floating,Qt::Key_F18,Qt::NoModifier,5,2);press(&floating,Qt::Key_QuoteLeft,Qt::NoModifier,5,2);
 check(expanded==2,"clearing native maximize assignment disables both old key and legacy backtick");
 QApplication::setActiveWindow(&main);typing.setFocus();QApplication::processEvents();
 press(&typing,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,5,2);check(calls==5,"modified native Edit command yields to text-field focus");
 typing.clearFocus();timeline->setFocus();
 QWidget pool(&main);pool.setObjectName("frameEditPageMediaPool");pool.setFocusPolicy(Qt::StrongFocus);pool.show();
 commands.addPanel("frameEditPageMediaPool","Media Pool",[]{return true;},[]{},[]{});
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}},{"MediaPool.editRename",{t}}}));settle();
 QApplication::setActiveWindow(&floating);timeline->setFocus();QApplication::processEvents();
 press(timeline,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,6,2);
 check(calls==6,"Timeline mapping shares a key with native Media Pool mapping");
 QApplication::setActiveWindow(&main);pool.setFocus();QApplication::processEvents();
 QKeyEvent poolOverride(QEvent::ShortcutOverride,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier);
 poolOverride.ignore();QApplication::sendEvent(&pool,&poolOverride);
 check(!poolOverride.isAccepted()&&calls==6,"outside Timeline OpenGate leaves shared key to native panel handling");
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}},{"EditTimeline.editOther",{t}}}));settle();
 QApplication::setActiveWindow(&floating);timeline->setFocus();QApplication::processEvents();
 press(timeline,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,6,2);
 check(calls==6,"native command in same Timeline scope remains a conflict");
 save("keyboard.preset.xml",nativePresetFixture({{stabilize,{t}},{"unqualifiedNative",{t}}}));settle();
 press(timeline,Qt::Key_T,Qt::AltModifier|Qt::ShiftModifier,6,2);
 check(calls==6,"unknown native scope is not silently whitelisted");
 if(failures){Unbound::flushDiagnostics();dir.setAutoRemove(false);std::fprintf(stderr,"Native preset diagnostics retained: %s; calls=%d expanded=%d active=%s\n",qPrintable(dir.path()),calls,expanded,qPrintable(QApplication::activeWindow()?QApplication::activeWindow()->objectName():QString("none")));}
 std::printf("native preset failures=%d\n",failures);return failures?1:0;
}
