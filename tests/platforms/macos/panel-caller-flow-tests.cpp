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
 QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);QTemporaryDir files(OpenGateTest::evidenceTemplate("panel-caller-flow-tests"));OpenGateTest::retainEvidence(files);
 const bool restart=false;
 if(!restart){qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());}qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
 int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.setObjectName("UiMainWindow");main.resize(1400,900);auto outer=new QVBoxLayout(&main);
 auto bar=new QMenuBar(&main);bar->setNativeMenuBar(false);outer->setMenuBar(bar);auto workspace=bar->addMenu("Workspace");workspace->setObjectName("workspace");
 auto toolbar=new QStackedWidget(&main);auto tools=new QWidget;tools->setObjectName("pageSingleScreenModeTools");toolbar->addWidget(tools);outer->addWidget(toolbar);auto toolrow=new QHBoxLayout(tools);auto left=new QWidget(tools);left->setObjectName("frameEditTopBarLeftSSM");toolrow->addWidget(left);auto leftrow=new QHBoxLayout(left);leftrow->addWidget(new QPushButton("Media Pool",left));auto right=new QWidget(tools);right->setObjectName("frameEditTopBarRightSSM");right->setLayout(new QHBoxLayout);toolrow->addWidget(right);
 auto pageStack=new QStackedWidget(&main);outer->addWidget(pageStack,1);auto page=new QWidget(pageStack);page->setObjectName("WStackPage_Conform");pageStack->addWidget(page);auto fusion=new QWidget(pageStack);fusion->setObjectName("WStackPage_Fusion");pageStack->addWidget(fusion);auto color=new QWidget(pageStack);color->setObjectName("WStackPage_Color");pageStack->addWidget(color);auto native=new QHBoxLayout(page);
 QHash<QString,QWidget *> nativePanels;for(const auto &id:{"frameConformTimelines","widgetEditSourcePlayer","widgetEditTimelinePlayer","frameEditPageMediaPool","frameEditPageEditIndex","widgetEditInspector","frameEditPageMetadata"}){
  auto panel=new QWidget(page);panel->setObjectName(id);auto layout=new QVBoxLayout(panel);layout->addWidget(new QPushButton(id,panel));native->addWidget(panel);nativePanels[id]=panel;if(QString(id)=="frameEditPageMetadata")panel->hide();
 }
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
 for(const auto &pair:QList<QPair<QString,QString>>{{"MediaPool","Media Pool"},{"EditIndex","Index"},{"Mixer","Mixer"}}){
  auto b=new QPushButton(pair.second,left);b->setCheckable(true);b->setObjectName("btnEditToggle"+pair.first+"InSSM");leftrow->addWidget(b);buttons[pair.first]=b;
 }
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
 check(buttons["MediaPool"]->isChecked()==checked("Media Pool") && buttons["EditIndex"]->isChecked()==checked("Index"),"header radio checks agree with selected managed tabs");
 check(buttons["Mixer"]->isChecked()&&checked("Mixer"),"visible nested Mixer is checked before it has its own session");
 const QJsonValue tree=binding->saved()["tree"];const auto nativeBefore=nativeCalls;
 buttons["EditIndex"]->setChecked(true);QTimer::singleShot(20,buttons["MediaPool"],[&]{buttons["MediaPool"]->setChecked(false);});settle(400);
 check(binding->saved()["tree"]==tree&&nativeCalls==nativeBefore,"deferred programmatic native radio state cannot cycle or select tabs");
 press(buttons["EditIndex"]);check(binding->selected("frameEditPageEditIndex")&&nativeCalls==nativeBefore,"one physical header click selects an inactive tab without operating native slots");
 auto detach=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool");if(detach)detach->click();settle(350);
 Unbound::ManagedPanelSession *pool=nullptr;for(auto session:controls.findChildren<QObject *>())if(auto candidate=dynamic_cast<Unbound::ManagedPanelSession *>(session))if(candidate->window()&&candidate->window()->objectName()=="unboundFloating_frameEditPageMediaPool")pool=candidate;
 check(pool&&pool->active(),"Media Pool owns a retained floating session");
 if(pool){
  auto host=pool->window();host->setGeometry(180,140,440,340);settle(150);const auto frame=host->geometry();
  press(buttons["MediaPool"]);check(pool->hiddenByUser()&&!buttons["MediaPool"]->isChecked(),"header hides floated panel and clears its check");
  call("Media Pool");check(pool->window()==host&&host->geometry()==frame&&!pool->hiddenByUser()&&!binding->contains("frameEditPageMediaPool"),"Panels recalls the same floating window at its position without docking");
  press(buttons["MediaPool"]);QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.toggle.frameEditPageMediaPool");settle(250);
  check(pool->window()==host&&host->geometry()==frame&&!pool->hiddenByUser(),"workflow shortcut recalls the same hidden float");
  call("Media Pool");press(buttons["MediaPool"]);check(pool->window()==host&&host->geometry()==frame&&!pool->hiddenByUser(),"header recalls the same hidden float");
 }
 const QJsonValue before=binding->saved()["tree"];settle(700);check(before==binding->saved()["tree"],"idle arrangement stays selected without automatic cycling");
 if(checked("Media Pool"))call("Media Pool");
 auto nativeReturn=ui?ui->findChild<QAction *>("unboundRestoreNativeWorkspace"):nullptr;if(nativeReturn)nativeReturn->trigger();settle(450);
 check(!binding->active()&&page->layout()==native,"Native return restores original layout after caller operations");
 check(buttons["MediaPool"]->isChecked()==nativePanels["frameEditPageMediaPool"]->isVisible()&&buttons["EditIndex"]->isChecked()==nativePanels["frameEditPageEditIndex"]->isVisible(),"Native return reflects native slot visibility rather than stale OpenGate door state");
 }
 if(failures){files.setAutoRemove(false);std::fprintf(stderr,"logs: %s\n",qPrintable(files.path()));}
 std::printf("panel caller flow failures=%d\n",failures);return failures?1:0;
}
