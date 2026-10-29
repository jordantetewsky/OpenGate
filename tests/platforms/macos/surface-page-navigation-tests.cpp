// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "footerpanel.h"
class NativePaintProbe : public QPushButton {
public:using QPushButton::QPushButton;
 void paintEvent(QPaintEvent *) override {QPainter p(this);p.fillRect(rect(),QColor(35,45,55));p.setPen(QColor(240,200,90));p.drawText(rect(),Qt::AlignCenter,"Native Edit");}
};
int main(int argc,char **argv){
 QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());int failures=0;
 auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
 auto settle=[] {QElapsedTimer t;t.start();while(t.elapsed()<140){QApplication::processEvents();QThread::msleep(1);}};
 {NativePaintProbe original;original.resize(120,36);original.setAttribute(Qt::WA_DontShowOnScreen);Unbound::NativePageButton mirror(nullptr);mirror.resize(original.size());mirror.setAttribute(Qt::WA_DontShowOnScreen);mirror.setNativeTarget(&original);
  for(int ratio:{1,2}){QImage a(original.size()*ratio,QImage::Format_ARGB32_Premultiplied),b(a.size(),a.format());a.setDevicePixelRatio(ratio);b.setDevicePixelRatio(ratio);a.fill(Qt::transparent);b.fill(Qt::transparent);original.render(&a);mirror.render(&b);check(a==b,"secondary native painting matches at normal and Retina density");}}
 QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1100,600);auto layout=new QVBoxLayout(&main);
 auto stack=new QStackedWidget(&main);layout->addWidget(stack);auto edit=new QWidget;edit->setObjectName("WStackPage_Conform");stack->addWidget(edit);auto audio=new QWidget;audio->setObjectName("WStackPage_Audio");stack->addWidget(audio);
 auto native=new QWidget(&main);native->setObjectName("m_PageSwitcherContainer");auto row=new QHBoxLayout(native);layout->addWidget(native);
 auto middle=new QWidget(native);middle->setObjectName("frameControlPanelMiddle");auto pages=new QHBoxLayout(middle);row->addWidget(middle);QPushButton *audioButton=nullptr;int pageClicks=0,homeClicks=0,settingsClicks=0;
 const QStringList ids{"buttonBrowsePanel","buttonFirstCutPanel","buttonConformPanel","buttonFusionPanel","buttonColorPanel","buttonPhotoPanel","buttonAudioPanel","buttonDeliverPanel"};
 for(auto id:ids){auto button=new QPushButton(id,middle);button->setObjectName(id);pages->addWidget(button);if(id=="buttonAudioPanel"){audioButton=button;QObject::connect(button,&QPushButton::clicked,[&]{++pageClicks;stack->setCurrentWidget(audio);});}}
 auto home=new QPushButton("Home",native);home->setObjectName("buttonHome");row->addWidget(home);QObject::connect(home,&QPushButton::clicked,[&]{++homeClicks;});auto settings=new QPushButton("Settings",native);settings->setObjectName("buttonConfig");row->addWidget(settings);QObject::connect(settings,&QPushButton::clicked,[&]{++settingsClicks;});
 auto action=new QAction("Show Page Navigation",&main);action->setCheckable(true);action->setChecked(true);QObject::connect(action,&QAction::toggled,[&](bool visible){native->setVisible(visible);});
 QWidget first(&main,Qt::Tool),second(&main,Qt::Tool);first.setObjectName("unboundFloating_first");second.setObjectName("unboundFloating_second");for(auto host:{&first,&second}){host->setAttribute(Qt::WA_DontShowOnScreen);host->resize(1000,500);auto box=new QVBoxLayout(host);box->addWidget(new QWidget(host));host->show();}main.show();settle();
 const auto parent=native->parentWidget();const auto index=layout->indexOf(native);const auto minimum=native->minimumSize();const auto policy=native->sizePolicy();QWidget *primary=&first;QJsonObject saved;
 {
 Unbound::FooterPanel controller(nullptr,[&]{return &main;},{});controller.primarySurface=[&]{return primary;};controller.surfaceHosts=[&]{return QList<QWidget *>{&first,&second};};controller.restore();settle();
 auto band=first.findChild<QScrollArea *>("unboundAttachedAccessory_m_PageSwitcherContainer");auto body=first.layout()->itemAt(0)->widget();
 check(band&&band->widget()==native&&native->window()==&first,"primary gets the actual complete native footer");check(home->window()==&first&&settings->window()==&first,"original Home and Settings travel with the real footer");check(body->geometry().bottom()<band->geometry().top(),"native footer reserves a bottom band outside content");home->click();settings->click();check(homeClicks==1&&settingsClicks==1,"real native Home and Settings signals stay connected");
 auto b=second.findChild<Unbound::NativeFooterMirror *>("openGateNativeFooterMirror");
 check(b&&b->isVisibleTo(&second),"secondary gets a reserved native footer without opting into pinning");
 auto canvas=b?static_cast<Unbound::NativeFooterCanvas *>(b->widget()):nullptr;
 auto mirrorClick=[&](QAbstractButton *button){
  const auto point=button->mapTo(native,button->rect().center());
  check(canvas && canvas->buttonAt(point)==button,"native coordinates resolve the original button");
  if(!canvas)return;
  QMouseEvent press(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
  QApplication::sendEvent(canvas,&press);QApplication::sendEvent(canvas,&release);settle();
 };
 mirrorClick(home);mirrorClick(settings);check(homeClicks==2&&settingsClicks==2,"complete footer mirror invokes native Home and Settings exactly once");
 for(int ratio:{1,2}) {
  QImage actual(native->size()*ratio,QImage::Format_ARGB32_Premultiplied),copy(actual.size(),actual.format());
  actual.setDevicePixelRatio(ratio);copy.setDevicePixelRatio(ratio);actual.fill(Qt::transparent);copy.fill(Qt::transparent);
  native->render(&actual);canvas->render(&copy);check(actual==copy,"complete footer layout and painting match native at normal and Retina density");
 }
 mirrorClick(audioButton);check(pageClicks==1&&stack->currentWidget()==audio&&audioButton->window()==&first,"page request retains real native owner and wiring");
 QWidget expansionHost;expansionHost.setAttribute(Qt::WA_DontShowOnScreen);
 check(!controller.externalBounds(&expansionHost).isEmpty() && controller.externalBounds(&first).isEmpty() && controller.externalBounds(&second).isEmpty(),"external expansion uses footer gate while hosts with their own band avoid double reservation");
 const int bodyHeight=body->height();controller.hidePanel();settle();check(!action->isChecked()&&band->isHidden()&&!b->isVisibleTo(&second)&&body->height()>bodyHeight,"hide removes reserved bands and content gains space");check(controller.externalBounds(&expansionHost).isEmpty(),"hidden footer removes maximize gate");
 action->trigger();settle();check(!band->isHidden()&&b->isVisibleTo(&second)&&body->height()==bodyHeight,"native visibility restores bands and original content bounds");
 second.resize(140,350);settle();check(b->horizontalScrollBar()->maximum()>0,"narrow native footer scrolls rather than truncates");
 b->horizontalScrollBar()->setValue(b->horizontalScrollBar()->maximum());settle();
 check(b->viewport()->rect().contains(canvas->mapTo(b->viewport(),settings->mapTo(native,settings->rect().center()))),"native Settings remains reachable by scrolling");
 primary=&main;controller.refreshSurfaces();settle();check(native->parentWidget()==parent&&layout->indexOf(native)==index,"native main return restores exact original slot");
 check(!main.findChild<Unbound::NativeFooterMirror *>("openGateNativeFooterMirror",Qt::FindDirectChildrenOnly),"native main uses original footer without replacement");
 controller.hidePanel();controller.showPanel();settle();check(!native->isHidden()&&action->isChecked()&&b->isVisibleTo(&second),"one visibility state controls native and secondary bands");
 saved=controller.snapshot();check(!saved["floating"].toBool()&&saved["model"]=="reserved-native-bands-v2"&&!saved.contains("surfaces"),"snapshot has no independent pin choices");
 }
 check(native->parentWidget()==parent&&layout->indexOf(native)==index&&native->minimumSize()==minimum&&native->sizePolicy()==policy,"shutdown restores native ownership and constraints");check(!first.findChild<QScrollArea *>("unboundAttachedAccessory_m_PageSwitcherContainer")&&!second.findChild<Unbound::NativeFooterMirror *>("openGateNativeFooterMirror"),"shutdown removes only owned wrappers and copies");
 {Unbound::FooterPanel restored(nullptr,[&]{return &main;},saved);restored.primarySurface=[&]{return &main;};restored.surfaceHosts=[&]{return QList<QWidget *>{&first,&second};};restored.restore();settle();check(restored.pinned(&second),"reserved secondary band survives restart");}
 {
 QWidget lateMain;lateMain.setAttribute(Qt::WA_DontShowOnScreen);auto layout=new QVBoxLayout(&lateMain);
 auto lateFooter=new QWidget(&lateMain);lateFooter->setObjectName("m_PageSwitcherContainer");layout->addWidget(lateFooter);lateMain.show();
 Unbound::FooterPanel delayed(nullptr,[&]{return &lateMain;},{{"hidden",true}});delayed.refreshSurfaces();settle();
 check(lateFooter->isHidden(),"saved hidden footer applies before native command exists");
 auto lateAction=new QAction("Show Page Navigation",&lateMain);lateAction->setCheckable(true);lateAction->setChecked(true);QObject::connect(lateAction,&QAction::toggled,lateFooter,&QWidget::setVisible);
 settle();delayed.refreshSurfaces();settle();
 check(!lateAction->isChecked()&&lateFooter->isHidden(),"late native command adopts saved hidden intent instead of overwriting it");
 QMenu lateMenu(&lateMain);lateMenu.setObjectName("workspaceShowPanel");delayed.wireMenu();
 auto lateCaller=lateMenu.findChild<QAction *>("unboundPageNavigation");check(lateCaller&&!lateCaller->isChecked(),"explicit wiring discovers a native caller menu created after initial discovery");
 if(lateCaller)lateCaller->trigger();settle();check(lateAction->isChecked()&&!lateFooter->isHidden(),"late caller controls the same native footer visibility");
 }
 std::printf("surface navigation failures=%d\n",failures);return failures?1:0;
}
