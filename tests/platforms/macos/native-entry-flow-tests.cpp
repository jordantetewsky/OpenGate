#include "../../core/workspace-callers/fixtureevidence.h"
#include "native-fixture-start.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QMenuBar>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelcontrols.h"
static void settle(int ms=250){QElapsedTimer timer;timer.start();while(timer.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
class HiddenDialogs : public QObject {
 bool eventFilter(QObject *object,QEvent *event) override {
  if(event->type()==QEvent::Polish)if(auto dialog=qobject_cast<QInputDialog *>(object))dialog->setAttribute(Qt::WA_DontShowOnScreen);
  return false;
 }
};
template<class T>T *component(QObject *owner){for(auto child:owner->children())if(auto value=dynamic_cast<T *>(child))return value;return nullptr;}
int main(int argc,char **argv) {
 QApplication app(argc,argv);HiddenDialogs hiddenDialogs;app.installEventFilter(&hiddenDialogs);app.setQuitOnLastWindowClosed(false);qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
 const bool restart=app.arguments().contains("--restart-check");QTemporaryDir files(OpenGateTest::evidenceTemplate("native-entry-flow-tests"));OpenGateTest::retainEvidence(files);
 if(!restart){qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());}
 int errors=0;auto check=[&](bool ok,const char *why){if(!ok){++errors;std::fprintf(stderr,"FAIL %s\n",why);}};
 QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1300,800);auto outer=new QVBoxLayout(&main);
 auto bar=new QMenuBar(&main);bar->setNativeMenuBar(false);auto workspace=bar->addMenu("Workspace");workspace->setObjectName("workspace");outer->setMenuBar(bar);
 auto toolbar=new QStackedWidget(&main);auto tools=new QWidget;tools->setObjectName("pageSingleScreenModeTools");toolbar->addWidget(tools);outer->addWidget(toolbar);auto row=new QHBoxLayout(tools);
 auto left=new QWidget(tools);left->setObjectName("frameEditTopBarLeftSSM");auto leftRow=new QHBoxLayout(left);auto nativeButton=new QPushButton("Native Media Pool",left);leftRow->addWidget(nativeButton);row->addWidget(left,1);
 auto title=new QPushButton("Project name retained",tools);title->setObjectName("btnProjectsMainBarProjectList");row->addWidget(title);auto right=new QWidget(tools);right->setObjectName("frameEditTopBarRightSSM");right->setLayout(new QHBoxLayout);row->addWidget(right);
 auto page=new QWidget(&main);page->setObjectName("WStackPage_Conform");outer->addWidget(page,1);auto native=new QHBoxLayout(page);
 auto surface=[&](const char *name){auto widget=new QWidget(page);widget->setObjectName(name);auto box=new QVBoxLayout(widget);auto header=new QWidget(widget);auto headerRow=new QHBoxLayout(header);auto options=new QToolButton(header);options->setObjectName("tbtnPlayerOptions");headerRow->addWidget(options);box->addWidget(header);box->addWidget(new QPushButton(name,widget),1);native->addWidget(widget);return widget;};
 auto timeline=surface("frameConformTimelines");auto source=surface("widgetEditSourcePlayer");auto program=surface("widgetEditTimelinePlayer");
 auto footer=new QWidget(&main);footer->setObjectName("m_PageSwitcherContainer");auto footRow=new QHBoxLayout(footer);outer->addWidget(footer);
 int homeCalls=0,settingsCalls=0;for(const auto &name:{"buttonConformPanel","buttonFusionPanel","buttonHome","buttonConfig"}){auto button=new QPushButton(name,footer);button->setObjectName(name);footRow->addWidget(button);if(QString(name)=="buttonHome")QObject::connect(button,&QPushButton::clicked,[&]{++homeCalls;});if(QString(name)=="buttonConfig")QObject::connect(button,&QPushButton::clicked,[&]{++settingsCalls;});}
 auto navigation=workspace->addAction("Show Page Navigation");navigation->setCheckable(true);navigation->setChecked(true);QObject::connect(navigation,&QAction::toggled,footer,&QWidget::setVisible);
 main.show();settle();
 if(!restart) {
  Unbound::ArrangementStore legacy;QString a,b;legacy.create("Native Emulator",{{"page","edit"}},a);legacy.create("Previous Test Layout",{{"page","edit"}},b);
  QFile file(qEnvironmentVariable("UNBOUND_ARRANGEMENTS_FILE"));file.open(QIODevice::ReadOnly);auto document=QJsonDocument::fromJson(file.readAll()).object();file.close();
  auto entries=document["arrangements"].toArray();for(int i=0;i<entries.size();++i){auto entry=entries[i].toObject();entry["hidden"]=true;entries[i]=entry;}document["arrangements"]=entries;
  file.open(QIODevice::WriteOnly);file.write(QJsonDocument(document).toJson());file.close();
 }
 {
 prepareNativeFixture();PanelControls controls;settle(600);auto registry=component<Unbound::PanelCallerMenu>(&controls);auto headers=component<Unbound::WorkspaceHeaders>(&controls);
 auto headerOwner=component<Unbound::PinnedWorkspaceHeader>(&controls);auto footerOwner=component<Unbound::FooterPanel>(&controls);
 auto ui=bar->findChild<QMenu *>("unboundWorkspaceUI");check(ui&&registry&&headers&&headerOwner&&footerOwner,"assembled native entry exposes menu and band owners");
 if(!ui||!registry||!headers||!headerOwner||!footerOwner)return 1;
 if(restart){check(source->isHidden(),"fresh process restores unsaved native visibility");check(!headerOwner->shown(),"fresh process restores hidden header choice");check(!navigation->isChecked(),"fresh process restores hidden footer choice through native command");std::printf("restart flow failures=%d\n",errors);return errors?1:0;}
 check(!controls.isVisible(),"native entry does not open Panel Manager");
 auto initialPresets=controls.findChild<QComboBox *>("arrangements");check(initialPresets&&initialPresets->count()==2,"clean start hides old arrangements with only the immutable Resolve Layout available");
 auto saveNew=ui->findChild<QAction *>("unboundSaveWorkspaceAs");check(saveNew&&ui->actions().contains(saveNew),"Save Arrangement As is directly available on clean start");
 auto oldStore=Unbound::ArrangementStore();check(oldStore.list().size()==2&&oldStore.hasHidden(),"hidden arrangement records remain stored");
 auto backgroundSwitch=controls.findChild<QCheckBox *>("hideResolveBackground");check(backgroundSwitch&&backgroundSwitch->isHidden()&&!backgroundSwitch->isChecked(),"manual background control is retained but hidden and off");
 auto sourceDoor=dynamic_cast<Unbound::NativeUndockButton *>(main.findChild<QToolButton *>("openGateToolbarDoor_widgetEditSourcePlayer"));
 check(sourceDoor&&bool(sourceDoor->drag),"Native header offers Source drag before any preset");
 QMenu flat;headers->populate(&flat,&main);bool sawHeader=false,sawFooter=false;
 for(auto action:flat.actions()){check(action->text()!="Edit"&&action->text()!="Workspace"&&action->text()!="All Panels","no category submenu in Panels");sawHeader|=action->text()=="Workspace Toolbar";sawFooter|=action->text()=="Page Navigation";}
 check(sawHeader&&sawFooter,"Panels directly exposes Header and Footer");
 auto menuBarPanels=ui->findChild<QMenu *>("openGateShowPanel");check(menuBarPanels&&menuBarPanels->findChildren<QMenu *>().isEmpty(),"menubar Panels has no page/category layers");
 left->setFixedWidth(nativeButton->sizeHint().width()+leftRow->contentsMargins().left()+leftRow->contentsMargins().right()+leftRow->spacing()+44);settle();
 auto callerStrip=tools->findChild<QWidget *>("openGateToolbarDoors");
 check(callerStrip&&callerStrip->parentWidget()==tools&&callerStrip->width()>28,"viewer callers use the existing toolbar's available space outside the bounded native button row");
 auto callerControls=callerStrip?callerStrip->findChild<QWidget *>("openGateDoorControls"):nullptr;
 check(sourceDoor&&sourceDoor->isVisible()&&sourceDoor->width()>=18&&callerControls&&callerControls->width()>=56,"viewer caller buttons receive actual usable layout width, not only a maximum");
 if(callerStrip)callerStrip->setFixedWidth(32);settle();auto overflow=main.findChild<QToolButton *>("openGateToolbarOverflow");
 check(overflow&&overflow->isVisible()&&overflow->menu()->actions().size()==2,"cramped native header retains both monitors in overflow");
 left->setMinimumWidth(0);left->setMaximumWidth(QWIDGETSIZE_MAX);if(callerStrip){callerStrip->setMinimumWidth(0);callerStrip->setMaximumWidth(QWIDGETSIZE_MAX);}settle();
 auto menuFor=[&]{auto menu=new QMenu(&main);registry->populateFlat(menu);return menu;};
 auto trigger=[&](const QString &label){auto menu=menuFor();bool found=false;for(auto action:menu->actions())if(action->text()==label){action->trigger();found=true;break;}settle();delete menu;check(found,qPrintable("Panels action "+label));};
 trigger("Source Monitor");check(source->isHidden()&&source->parentWidget()==page,"Panels Hide retains native source ownership");
 if(sourceDoor)sourceDoor->click();settle();check(!source->isHidden()&&source->parentWidget()==page,"header Show restores same native source without floating");
 trigger("Program Monitor");check(program->isHidden()&&program->parentWidget()==page,"program visibility keeps native ownership");trigger("Program Monitor");
 auto pageHeight=page->height();trigger("Workspace Toolbar");trigger("Page Navigation");check(toolbar->isHidden()&&footer->isHidden()&&page->height()>pageHeight,"both hidden bands release native workspace space");
 QMenu stillAccessible;headers->populate(&stillAccessible,&main);check(!stillAccessible.actions().isEmpty(),"Panels remains populated with both bands hidden");
 trigger("Workspace Toolbar");trigger("Page Navigation");check(!toolbar->isHidden()&&!footer->isHidden()&&page->height()==pageHeight,"Show restores original bands and content bounds");
 check(title->text()=="Project name retained"&&nativeButton->parentWidget()==left,"native header controls and title retained");
 if(sourceDoor&&sourceDoor->drag)sourceDoor->drag(sourceDoor->mapToGlobal(QPoint()));settle(500);
 auto floating=dynamic_cast<Unbound::WorkspaceFloatingHost *>(source->window());check(floating&&floating!=&main,"native header drag callback detaches source directly");
 if(floating&&floating->attachRequested)floating->attachRequested(timeline,Qt::RightEdge);settle(450);
 check(source->window()==&main&&page->findChild<QWidget *>("unboundNativePanelPosition_frameConformTimelines"),"native first drop creates a scoped arrangement");
 auto expand=controls.findChild<QPushButton *>("expand_widgetEditSourcePlayer");if(expand)expand->click();settle(500);
 auto expanded=source->window();const auto expandedGeometry=expanded->geometry();
 auto matchesViewport=[&](QWidget *host,QWidget *owner,QWidget *nativeBody){
  const auto margins=host->windowHandle()?host->windowHandle()->frameMargins():QMargins();
  const auto expected=Unbound::workspaceViewportBounds(owner,nativeBody).adjusted(margins.left(),margins.top(),-margins.right(),-margins.bottom());
  return host->geometry()==expected;
 };
 check(expanded!=&main&&matchesViewport(expanded,&main,page),"maximize exactly fills native workspace content rather than monitor");
 trigger("Workspace Toolbar");const auto withoutHeader=expanded->geometry();check(withoutHeader.top()<expandedGeometry.top()&&withoutHeader.bottom()==expandedGeometry.bottom()&&matchesViewport(expanded,&main,page),"hiding only header removes only top gate");
 trigger("Page Navigation");check(expanded->geometry().bottom()>withoutHeader.bottom()&&matchesViewport(expanded,&main,page),"hiding footer removes bottom gate too");
 trigger("Workspace Toolbar");check(expanded->geometry().top()==expandedGeometry.top()&&expanded->geometry().bottom()>expandedGeometry.bottom()&&matchesViewport(expanded,&main,page),"showing only header reinstates only top gate");
 trigger("Page Navigation");check(expanded->geometry()==expandedGeometry&&matchesViewport(expanded,&main,page),"showing both restores exact workspace bounds");
 const auto originalMainSize=main.size();main.resize(main.width()+120,main.height()+80);settle(450);
 check(matchesViewport(expanded,&main,page),"current maximize follows workspace resizing");main.resize(originalMainSize);settle(450);
 if(expand)expand->click();settle(400);check(source->window()==&main,"maximize restore returns scoped group");
 auto save=ui->findChild<QAction *>("unboundSaveWorkspaceAs");check(save,"save arrangement accessible outside manager");
 QTimer answer;answer.setInterval(10);QObject::connect(&answer,&QTimer::timeout,[&]{for(auto widget:QApplication::topLevelWidgets())if(auto dialog=qobject_cast<QInputDialog *>(widget)){dialog->setTextValue("Native entry fixture");dialog->accept();}});answer.start();if(save)save->trigger();settle(500);answer.stop();
 auto presets=controls.findChild<QComboBox *>("arrangements");check(presets&&presets->findText("Native entry fixture")>=0,"public Save As preserves named arrangement");
 auto nativeReturn=ui->findChild<QAction *>("unboundRestoreNativeWorkspace");check(nativeReturn,"Native Edit recovery action available");if(nativeReturn)nativeReturn->trigger();settle(600);
 check(source->parentWidget()==page&&timeline->parentWidget()==page&&!page->findChild<QWidget *>("unboundNativePanelPosition_frameConformTimelines"),"Native Edit returns original slots after direct rearrangement");
 const auto savedId=presets?presets->currentData().toString():QString();auto savedRestore=ui->findChild<QAction *>("unboundWorkspaceRestore_"+savedId);
 check(savedRestore,"named arrangement remains directly selectable after Native return");if(savedRestore)savedRestore->trigger();settle(650);
 check(page->findChild<QWidget *>("unboundNativePanelPosition_frameConformTimelines")&&source->window()==&main,"saved arrangement restores original scoped group through menu");
 if(nativeReturn)nativeReturn->trigger();settle(550);
 auto programDoor=dynamic_cast<Unbound::NativeUndockButton *>(main.findChild<QToolButton *>("openGateToolbarDoor_widgetEditTimelinePlayer"));
 auto timelineDoor=dynamic_cast<Unbound::NativeUndockButton *>(main.findChild<QToolButton *>("unboundTimelineUndock"));
 check(programDoor&&programDoor->drag&&timelineDoor&&timelineDoor->drag,"remaining native controls expose direct drag callbacks");
 if(sourceDoor&&sourceDoor->drag)sourceDoor->drag(QPoint());settle(450);
 if(programDoor&&programDoor->drag)programDoor->drag(QPoint());settle(450);
 if(timelineDoor&&timelineDoor->drag)timelineDoor->drag(QPoint());settle(450);
 auto timelineHost=timeline->window();auto programHost=dynamic_cast<Unbound::WorkspaceFloatingHost *>(program->window());
 check(timelineHost!=&main&&toolbar->window()==&main&&footer->window()==&main,"fully rearranged panels never take primary workspace bands");
 const auto primaryFrame=main.geometry();
 auto expandTimeline=controls.findChild<QPushButton *>("expand_frameConformTimelines");if(expandTimeline)expandTimeline->click();settle(600);
 check(main.geometry()==primaryFrame&&matchesViewport(timeline->window(),&main,page),"standalone maximize fills primary content without resizing its owner");
 trigger("Workspace Toolbar");trigger("Page Navigation");check(main.geometry()==primaryFrame&&matchesViewport(timeline->window(),&main,page),"primary frame stays fixed while maximize reclaims hidden band space");
 trigger("Workspace Toolbar");trigger("Page Navigation");check(main.geometry()==primaryFrame&&matchesViewport(timeline->window(),&main,page),"showing bands restores primary content bounds without window shrink");
 if(expandTimeline)expandTimeline->click();settle(450);
 if(programHost&&programHost->attachRequested)programHost->attachRequested(timelineHost,Qt::RightEdge);settle(550);
 check(timeline->window()==program->window()&&toolbar->window()==&main&&footer->window()==&main,"grouping leaves native bands with the primary workspace");
 check(!timeline->isAncestorOf(toolbar)&&!timeline->isAncestorOf(footer)&&!program->isAncestorOf(toolbar)&&!program->isAncestorOf(footer),"native bands never become borrowed panel leaf content");
 auto floatingWorkspace=timeline->window();const auto floatingFrame=floatingWorkspace->geometry();
 auto expandProgram=controls.findChild<QPushButton *>("expand_widgetEditTimelinePlayer");if(expandProgram)expandProgram->click();settle(500);
 auto expandedProgram=program->window();check(expandedProgram!=floatingWorkspace&&matchesViewport(expandedProgram,floatingWorkspace,nullptr),"floating group maximize fills that group's own content viewport");
 trigger("Workspace Toolbar");trigger("Page Navigation");check(matchesViewport(expandedProgram,floatingWorkspace,nullptr),"primary band visibility does not change the floating group content viewport");
 trigger("Workspace Toolbar");trigger("Page Navigation");check(matchesViewport(expandedProgram,floatingWorkspace,nullptr)&&floatingWorkspace->geometry()==floatingFrame,"restoring bands preserves floating workspace frame");
 if(expandProgram)expandProgram->click();settle(450);check(program->window()==floatingWorkspace,"floating group restore returns to retained workspace");

 if(nativeReturn)nativeReturn->trigger();settle(650);
 check(timeline->parentWidget()==page&&source->parentWidget()==page&&program->parentWidget()==page&&toolbar->parentWidget()==&main&&footer->parentWidget()==&main,"Native Edit restores native panel and band slots after full rearrangement");
 auto reveal=ui->findChild<QAction *>("openGateShowHiddenArrangements");check(reveal&&reveal->isEnabled(),"Advanced can recover hidden arrangements");
 if(reveal)reveal->trigger();settle(350);check(initialPresets->count()==5&&source->parentWidget()==page,"revealing saved arrangements preserves current native layout and new preset");
 trigger("Source Monitor");trigger("Workspace Toolbar");trigger("Page Navigation");settle(500);
 }
 QProcess child;child.setProgram(app.applicationFilePath());child.setArguments({"--restart-check"});child.start();check(child.waitForFinished(30000),"fresh-process restart fixture completes");std::fprintf(stderr,"%s",child.readAllStandardError().constData());check(child.exitStatus()==QProcess::NormalExit&&child.exitCode()==0,"unsaved native changes survive actual process restart");
 std::printf("native entry flow failures=%d\n",errors);return errors?1:0;
}
