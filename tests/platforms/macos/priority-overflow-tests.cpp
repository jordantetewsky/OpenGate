// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "panelsession.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
 int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
 auto settle=[] {QElapsedTimer t;t.start();while(t.elapsed()<80){QApplication::processEvents();QThread::msleep(1);}};
 for(const auto &id:QStringList{"widgetEditInspector","frameEditPageMediaPool","frameEditToolbarTop","widgetEditSourcePlayer","widgetEditTimelinePlayer","stackedEditSourceOfflinePlayers"}) {
  QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1000,500);auto outer=new QVBoxLayout(&main);
  auto content=new QWidget(&main);content->setObjectName(id);content->setMinimumWidth(760);auto row=new QHBoxLayout(content);
  auto first=new QPushButton("First",content);auto last=new QPushButton("Last",content);row->addWidget(first);row->addStretch();row->addWidget(last);outer->addWidget(content);
  main.show();settle();const auto minimum=content->minimumSize();const auto policy=content->sizePolicy();
  Unbound::MacPanelSession session;check(session.undock(content,&main,id).isEmpty(),"detach overflow target");
  auto host=session.window();if(!host){continue;}host->setAttribute(Qt::WA_DontShowOnScreen);
  auto area=host->findChild<QScrollArea *>();check(area,"every targeted panel gets an overflow viewport");
  if(area)for(double scale:{0.8,1.0,1.25}) {
   session.setInterfaceScale(scale);host->resize(320,300);settle();
   check(area->horizontalScrollBar()->maximum()>0,"narrow panel exposes horizontal range at each scale");
   auto right=area->findChild<QToolButton *>("openGateScrollRight");check(right&&right->isVisibleTo(area),"overflow has visible direction cue");
   area->horizontalScrollBar()->setValue(area->horizontalScrollBar()->maximum());settle();
   check(area->viewport()->rect().contains(last->mapTo(area->viewport(),last->rect().center())),"last control remains reachable");
   check(last->isEnabled(),"last control remains enabled");
   area->horizontalScrollBar()->setValue(0);settle();
  }
  session.setInterfaceScale(1.0);check(session.dock().isEmpty(),"redock overflow panel");settle();
  check(content->parentWidget()==&main&&content->minimumSize()==minimum&&content->sizePolicy()==policy,"native parent and constraints restored");
 }
 std::printf("priority overflow failures=%d\n",failures);return failures?1:0;
}
