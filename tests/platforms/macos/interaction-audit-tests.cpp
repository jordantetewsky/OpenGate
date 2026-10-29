// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <cstdio>
#include <QMimeData>
#include <QDropEvent>
#include <QMouseEvent>
#include "interactiondiagnostics.h"
class NativeInputReceiver : public QWidget {
public:
 using QWidget::QWidget;
 int drops=0,doubleClicks=0;
 bool event(QEvent *event) override {
  if(event->type()==QEvent::DragEnter){event->accept();return true;}
  if(event->type()==QEvent::Drop){++drops;event->accept();return true;}
  if(event->type()==QEvent::MouseButtonDblClick){++doubleClicks;event->accept();return true;}
  return QWidget::event(event);
 }
};
int main(int argc,char **argv) {
 const bool trace=argc<2 || QByteArray(argv[1])!="--trace-off";
 qputenv("OPENGATE_TRACE_NATIVE_LOADING",trace?"1":"0");
 QApplication app(argc,argv); QTemporaryDir dir; qputenv("UNBOUND_DIAGNOSTICS_DIR",dir.path().toUtf8());
 QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen);main.setObjectName("UiMainWindow");
 auto layout=new QVBoxLayout(&main);auto stack=new QStackedWidget;layout->addWidget(stack);
 auto edit=new QWidget;edit->setObjectName("WStackPage_Conform");stack->addWidget(edit);
 auto fusion=new QWidget;fusion->setObjectName("WStackPage_Fusion");stack->addWidget(fusion);
 auto hidden=new QPushButton("Hidden native command",edit);hidden->setObjectName("hiddenNative");hidden->hide();
 QPushButton poolToggle(edit);poolToggle.setObjectName("btnEditToggleMediaPoolInSSM");poolToggle.setCheckable(true);
 QObject::connect(&poolToggle,&QAbstractButton::toggled,&poolToggle,[&](bool checked) {
  if(checked)QTimer::singleShot(0,&poolToggle,[&] {QSignalBlocker block(&poolToggle);poolToggle.setChecked(false);});
 });
 NativeInputReceiver source(edit);source.setObjectName("widgetEditSourcePlayer");source.setAcceptDrops(true);
 Unbound::InteractionDiagnostics diagnostics(nullptr);main.show();QApplication::processEvents();
 // A native player created after initial discovery must receive an inventory
 // via the deferred affected-subtree scan, without another main-window Show.
 NativeInputReceiver latePlayer(edit);latePlayer.setObjectName("widgetEditTimelinePlayer");latePlayer.show();QApplication::processEvents();
 poolToggle.setChecked(true);QApplication::processEvents();
 QMimeData clip;clip.setText("private-fixture-clip-payload");
 QDragEnterEvent enter(QPoint(2,2),Qt::CopyAction,&clip,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&source,&enter);
 QDropEvent drop(QPointF(2,2),Qt::CopyAction,&clip,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&source,&drop);
 QMouseEvent doubleClick(QEvent::MouseButtonDblClick,QPointF(2,2),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&source,&doubleClick);
 hidden->click();auto dynamic=new QPushButton("Dynamic hidden command",edit);dynamic->setObjectName("dynamicNative");dynamic->hide();
 QApplication::processEvents();dynamic->click();
 auto action=new QAction("Dynamic native action",&main);action->setObjectName("dynamicNativeAction");main.addAction(action);action->trigger();
 stack->setCurrentWidget(fusion);QApplication::processEvents();
 Unbound::flushDiagnostics();QFile file(dir.path()+"/events.jsonl");file.open(QIODevice::ReadOnly);
 int hiddenCount=0,dynamicCount=0,actionCount=0,navigationCount=0,lateInventories=0,toggleObserved=0,toggleSettled=0;bool fusionSeen=false,ok=true;QString session;double sequence=0;
 while(!file.atEnd()) {auto line=file.readLine();ok &= !line.contains("private-fixture-clip-payload");auto record=QJsonDocument::fromJson(line).object();
  auto id=record["session"].toString();if(session.isEmpty())session=id;
  ok &= !id.isEmpty() && id==session && record["event_sequence"].toDouble()>sequence;sequence=record["event_sequence"].toDouble();
  if(record["event"]=="viewer.native-methods" && record["object"].toObject()["name"]=="widgetEditTimelinePlayer") {
   ++lateInventories;ok &= !record["native_receiver_execution_observed"].toBool();
   const auto methods=record["methods"].toArray();ok &= methods.size()==5;
   for(auto method:methods)ok &= method.toObject()["status"]=="not-present-in-runtime-metaobject";
  }
  if(record["event"]=="native.toggle.observed") {++toggleObserved;ok &= record["emitted_checked"].toBool();}
  if(record["event"]=="native.toggle.next-turn") {++toggleSettled;ok &= record["emitted_checked"].toBool()&&!record["current_checked"].toBool();}
  if(record["event"]=="input.navigation.delivery") {++navigationCount;ok &= record["phase"]=="before-native-handler";}
  if(record["event"]=="input.button.clicked") {auto name=record["button"].toObject()["name"].toString();
   if(name=="hiddenNative")++hiddenCount;if(name=="dynamicNative")++dynamicCount;
   ok &= record["page"]=="edit" && record["observation"]=="qt-button-signal";
  }
  if(record["event"]=="input.action.triggered" && record["action"]=="dynamicNativeAction")++actionCount;
  if(record["event"]=="workspace.page.changed" && record["page"]=="fusion")fusionSeen=true;
 }
 ok &= toggleObserved==(trace?1:0) && toggleSettled==(trace?1:0);
 ok &= lateInventories==(trace?1:0);
 ok &= navigationCount==(trace?3:0) && source.drops==1 && source.doubleClicks==1 && drop.isAccepted() && doubleClick.isAccepted();
 ok &= hiddenCount==1 && dynamicCount==1 && actionCount==1 && fusionSeen;
 std::printf("interaction audit %s hidden=%d dynamic=%d fusion=%d\n",ok?"PASS":"FAIL",hiddenCount,dynamicCount,fusionSeen);return ok?0:1;
}
