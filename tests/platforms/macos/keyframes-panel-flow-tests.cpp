#include "../../core/workspace-callers/fixtureevidence.h"
#include <QApplication>
#include <QButtonGroup>
#include <QElapsedTimer>
#include <QMenuBar>
#include <QProcess>
#include <QInputDialog>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelcontrols.h"
static void settle(int ms=600){QElapsedTimer t;t.start();while(t.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}}
template<class T>T *component(QObject *owner){for(auto child:owner->children())if(auto v=dynamic_cast<T *>(child))return v;return nullptr;}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);QTemporaryDir files(OpenGateTest::evidenceTemplate("opengate-keyframes-flow-tests"));OpenGateTest::retainEvidence(files);
 const bool restart=app.arguments().contains("--restart-check");
 if(!restart){qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());}qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
 int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.setObjectName("UiMainWindow");main.resize(1400,900);auto outer=new QVBoxLayout(&main);
 auto bar=new QMenuBar(&main);bar->setNativeMenuBar(false);outer->setMenuBar(bar);auto workspace=bar->addMenu("Workspace");workspace->setObjectName("workspace");
 auto toolbar=new QStackedWidget(&main);auto tools=new QWidget;tools->setObjectName("pageSingleScreenModeTools");toolbar->addWidget(tools);outer->addWidget(toolbar);auto toolrow=new QHBoxLayout(tools);auto left=new QWidget(tools);left->setObjectName("frameEditTopBarLeftSSM");toolrow->addWidget(left);auto leftrow=new QHBoxLayout(left);leftrow->addWidget(new QPushButton("Media Pool",left));auto right=new QWidget(tools);right->setObjectName("frameEditTopBarRightSSM");right->setLayout(new QHBoxLayout);toolrow->addWidget(right);
 auto pageStack=new QStackedWidget(&main);outer->addWidget(pageStack,1);auto page=new QWidget(pageStack);page->setObjectName("WStackPage_Conform");pageStack->addWidget(page);auto fusion=new QWidget(pageStack);fusion->setObjectName("WStackPage_Fusion");pageStack->addWidget(fusion);auto color=new QWidget(pageStack);color->setObjectName("WStackPage_Color");pageStack->addWidget(color);auto native=new QHBoxLayout(page);
 QHash<QString,QWidget *> nativePanels;for(const auto &id:{"frameConformTimelines","widgetEditSourcePlayer","widgetEditTimelinePlayer","frameEditPageMediaPool","frameEditPageEditIndex","widgetEditInspector","frameEditPageMetadata"}){
  auto panel=new QWidget(page);panel->setObjectName(id);auto layout=new QVBoxLayout(panel);layout->addWidget(new QPushButton(id,panel));native->addWidget(panel);nativePanels[id]=panel;if(QString(id)=="frameEditPageMetadata")panel->hide();
 }
 auto keyframes=new QWidget(page);keyframes->setObjectName("frameEditPageKeyframeEditor");auto keySlot=new QVBoxLayout(keyframes);native->addWidget(keyframes);keyframes->hide();
 auto editor=new QWidget(keyframes);editor->setObjectName("widgetKeyframeCurveEditor");auto editorLayout=new QVBoxLayout(editor);keySlot->addWidget(editor);
 auto keyTools=new QWidget(editor);keyTools->setObjectName("keyframeToolbarButtons");auto keyRow=new QHBoxLayout(keyTools);editorLayout->addWidget(keyTools);
 auto keyOptions=new QToolButton(keyTools);keyOptions->setObjectName("btnCurveSettingsOptions");keyRow->addWidget(keyOptions);
 auto nativeDetach=new QPushButton("Float Keyframes",keyTools);nativeDetach->setObjectName("btnUndock");keyRow->addWidget(nativeDetach);editorLayout->addWidget(new QWidget(editor),1);
 int nativeKeyframeDetaches=0;QObject::connect(nativeDetach,&QPushButton::clicked,[&]{++nativeKeyframeDetaches;});
 auto mixer=new QWidget(nativePanels["frameConformTimelines"]);mixer->setObjectName("frameEditAudioMeter");
 auto mixerLayout=new QVBoxLayout(mixer);auto mixerRow=new QWidget(mixer);auto mixerTools=new QHBoxLayout(mixerRow);
 auto mixerTitle=new QLabel("Mixer",mixerRow);mixerTools->addWidget(mixerTitle);auto mixerOptions=new QToolButton(mixerRow);mixerOptions->setObjectName("btnAudioMixerOptions");mixerTools->addWidget(mixerOptions);mixerLayout->addWidget(mixerRow);
 nativePanels["frameConformTimelines"]->layout()->addWidget(mixer);
 auto footer=new QWidget(&main);footer->setObjectName("m_PageSwitcherContainer");outer->addWidget(footer);auto footrow=new QHBoxLayout(footer);for(const auto &id:{"buttonConformPanel","buttonFusionPanel","buttonHome","buttonConfig"}){auto b=new QPushButton(id,footer);b->setObjectName(id);footrow->addWidget(b);}
 auto navigation=workspace->addAction("Show Page Navigation");navigation->setCheckable(true);navigation->setChecked(true);QObject::connect(navigation,&QAction::toggled,footer,&QWidget::setVisible);
 // Resolve's toolbar lives inside the page tree that gets parked, not as a
 // sibling supplied by our fixture. Keep that ownership topology realistic.
 outer->removeWidget(toolbar);native->insertWidget(0,toolbar);
 QHash<QString,QPushButton *> buttons;
 for(const auto &pair:QList<QPair<QString,QString>>{{"MediaPool","Media Pool"},{"EditIndex","Index"},{"Mixer","Mixer"},{"KeyframeEditor","Keyframes"}}){
  auto b=new QPushButton(pair.second,left);b->setCheckable(true);b->setObjectName("btnEditToggle"+pair.first+"InSSM");leftrow->addWidget(b);buttons[pair.first]=b;
 }
 QObject::connect(buttons["KeyframeEditor"],&QPushButton::clicked,[&](bool checked){QTimer::singleShot(0,keyframes,[=]{keyframes->setVisible(checked);});});
 QButtonGroup radio;radio.addButton(buttons["MediaPool"]);radio.addButton(buttons["EditIndex"]);radio.setExclusive(true);
 int nativeCalls=0;for(auto button:buttons)QObject::connect(button,&QPushButton::clicked,[&]{++nativeCalls;});
 main.show();settle();
 {
 PanelControls controls;settle(1800);auto binding=component<Unbound::NativeWorkspaceBinding>(&controls);
 auto callers=component<Unbound::PanelCallerMenu>(&controls);auto ui=bar->findChild<QMenu *>("unboundWorkspaceUI");
 auto checked=[&](const QString &title){QMenu menu;callers->populateFlat(&menu);for(auto action:menu.actions())if(action->text()==title)return action->isChecked();check(false,"caller exists");return false;};
 auto call=[&](const QString &title){QMenu menu;callers->populateFlat(&menu);for(auto action:menu.actions())if(action->text()==title){action->trigger();break;}settle(250);};
 auto press=[&](QPushButton *button){const auto p=button->rect().center();QMouseEvent down(QEvent::MouseButtonPress,p,button->mapToGlobal(p),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(button,&down);QMouseEvent up(QEvent::MouseButtonRelease,p,button->mapToGlobal(p),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(button,&up);settle(250);};
 check(binding&&binding->active(),"assembled first launch mounts Resolve Layout");
 if(restart){
  Unbound::ManagedPanelSession *restored=nullptr;for(auto object:controls.children())if(auto session=dynamic_cast<Unbound::ManagedPanelSession *>(object))if(session->panelTitle()=="Keyframes")restored=session;
  check(restored&&restored->active()&&restored->grouped()&&editor->window()!=&main,"restart reacquires Keyframes into its saved floating group");
  check(checked("Keyframes")==buttons["KeyframeEditor"]->isChecked(),"restart synchronizes Keyframes header and Panels state");
  return failures?1:0;
 }
 check(!checked("Keyframes")&&!buttons["KeyframeEditor"]->isChecked(),"optional Keyframes starts hidden in both callers");
 press(buttons["KeyframeEditor"]);
 check(binding->contains("frameEditPageKeyframeEditor")&&checked("Keyframes")&&buttons["KeyframeEditor"]->isChecked(),"header acquires real Keyframes content into OpenGate instead of exposing parked native UI");
 check(editor->isVisible()&&editor->window()==&main&&keyframes->isHidden(),"Keyframes uses existing content lease and leaves no native placeholder");
 press(nativeDetach);
 Unbound::ManagedPanelSession *keySession=nullptr;for(auto session:controls.findChildren<QObject *>())if(auto candidate=dynamic_cast<Unbound::ManagedPanelSession *>(session))if(candidate->window()&&candidate->window()->objectName()=="unboundFloating_frameEditPageKeyframeEditor")keySession=candidate;
 check(nativeKeyframeDetaches==0,"native Keyframes float control uses OpenGate placement instead of a parallel native tray");
 check(keySession&&keySession->floating()&&!binding->contains("frameEditPageKeyframeEditor"),"Keyframes float control detaches the existing panel session");
 if(keySession){
  press(nativeDetach);check(binding->contains("frameEditPageKeyframeEditor")&&editor->window()==&main&&checked("Keyframes"),"floating Keyframes docks into the visible OpenGate arrangement");
  auto keyDoor=buttons["KeyframeEditor"];const auto dragPoint=keyDoor->rect().center();
  QMouseEvent down(QEvent::MouseButtonPress,dragPoint,keyDoor->mapToGlobal(dragPoint),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(keyDoor,&down);
  QMouseEvent move(QEvent::MouseMove,dragPoint+QPoint(90,0),keyDoor->mapToGlobal(dragPoint+QPoint(90,0)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(keyDoor,&move);
  QMouseEvent up(QEvent::MouseButtonRelease,dragPoint+QPoint(90,0),keyDoor->mapToGlobal(dragPoint+QPoint(90,0)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(keyDoor,&up);settle(350);
  check(keySession->floating(),"dragging the native Keyframes title floats its existing session without firing a door click");
  auto host=keySession->window();host->setGeometry(160,150,480,340);settle(150);const auto frame=host->geometry();
  call("Keyframes");press(buttons["KeyframeEditor"]);check(keySession->window()==host&&host->geometry()==frame&&checked("Keyframes"),"Keyframes hide/recall preserves floating window and position");
  QApplication::setActiveWindow(&main);check(controls.invokeWorkflowCommand("panel.expand.frameEditPageKeyframeEditor"),"registered Keyframes maximize command dispatches");settle(300);
  check(keySession->window()->geometry()!=frame,"Keyframes uses ordinary workspace maximize geometry");
  auto matchesViewport=[&]{auto expanded=keySession->window();const auto margins=expanded->windowHandle()?expanded->windowHandle()->frameMargins():QMargins();return expanded->geometry()==Unbound::workspaceViewportBounds(&main,page).adjusted(margins.left(),margins.top(),-margins.right(),-margins.bottom());};
  check(matchesViewport(),"Keyframes maximizes to the existing workspace content viewport");
  for(const auto &band:{"Workspace Toolbar","Page Navigation","Workspace Toolbar","Page Navigation"}){call(band);check(matchesViewport(),"Keyframes maximized bounds follow every toolbar/navigation visibility combination");}

  QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameEditPageKeyframeEditor");settle(300);
  check(keySession->window()==host&&host->geometry()==frame,"Keyframes maximize restore retains exact floating placement");
  auto detachPool=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool");if(detachPool)detachPool->click();settle(300);
  auto poolHost=main.findChild<Unbound::WorkspaceFloatingHost *>("unboundFloating_frameEditPageMediaPool");
  auto keyHost=dynamic_cast<Unbound::WorkspaceFloatingHost *>(host);if(keyHost&&poolHost&&keyHost->attachRequested)keyHost->attachRequested(poolHost,Qt::Edge(0));settle(350);
  check(keySession->grouped()&&keySession->window()->window()!=&main,"Keyframes can tab with another floating panel through the existing controller");
  call("Media Pool");if(!checked("Keyframes"))press(buttons["KeyframeEditor"]);settle(200);
  auto groupHost=keySession->window()->window();const auto groupFrame=groupHost->geometry();
  QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameEditPageKeyframeEditor");settle(300);
  QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameEditPageKeyframeEditor");settle(300);
  check(keySession->grouped()&&keySession->window()->window()==groupHost&&groupHost->geometry()==groupFrame,"grouped Keyframes maximize restores its same group and frame");
  const auto keyParent=editor->parentWidget();pageStack->setCurrentWidget(color);settle(250);pageStack->setCurrentWidget(page);settle(500);
  check(keySession->grouped()&&keySession->window()->window()==groupHost&&editor->parentWidget()==keyParent,"page departure and return preserve Keyframes group and content owner");
  check(checked("Keyframes")==buttons["KeyframeEditor"]->isChecked(),"page return resynchronizes the Keyframes header check");
  settle(300);QProcess child;child.setProgram(app.applicationFilePath());child.setArguments({"--restart-check"});child.start();check(child.waitForFinished(30000),"Keyframes restart fixture completes");std::fprintf(stderr,"%s",child.readAllStandardError().constData());check(child.exitStatus()==QProcess::NormalExit&&child.exitCode()==0,"Keyframes saved placement survives actual process restart");

 }
 auto nativeReturn=ui?ui->findChild<QAction *>("unboundRestoreNativeWorkspace"):nullptr;if(nativeReturn)nativeReturn->trigger();settle(450);
 check(!binding->active()&&page->layout()==native&&editor->parentWidget()==keyframes,"Native return restores Keyframes layout and original content owner");
 auto selections=ui?ui->findChild<QMenu *>("openGateWorkspaces"):nullptr;
 if(selections&&!selections->actions().isEmpty())selections->actions().first()->trigger();settle(1800);
 check(binding->active()&&buttons["KeyframeEditor"]->isChecked()==checked("Keyframes"),"Native to OpenGate restoration completes with consistent Keyframes caller state");
 }
 if(failures){files.setAutoRemove(false);std::fprintf(stderr,"logs: %s\n",qPrintable(files.path()));}
 std::printf("Keyframes panel flow failures=%d\n",failures);return failures?1:0;
}
