// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QElapsedTimer>
#include <QThread>
#include <QMouseEvent>
#include <cstdio>
#include "../../../src/core/nativetabviewport.h"
#include "panelmetricscale.h"
int main(int argc,char **argv){
 QApplication app(argc,argv);int failures=0;auto check=[&](bool ok,const char *message){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}};
 auto settle=[] {QElapsedTimer t;t.start();while(t.elapsed()<100){QApplication::processEvents();QThread::msleep(1);}};
 QWidget surface;surface.setAttribute(Qt::WA_DontShowOnScreen);surface.resize(400,250);auto outer=new QVBoxLayout(&surface);auto row=new QWidget(&surface);row->setFixedHeight(32);auto layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);
 auto tabs=new QTabBar(row);tabs->setUsesScrollButtons(true);tabs->setExpanding(false);tabs->setMovable(true);tabs->setElideMode(Qt::ElideNone);layout->addWidget(tabs);auto plus=new QPushButton("+",row);plus->setFixedWidth(30);layout->addWidget(plus);outer->addWidget(row);auto ruler=new QWidget(&surface);outer->addWidget(ruler,1);
 for(int i=0;i<18;++i)tabs->addTab(QString("Timeline %1 long title").arg(i));surface.show();settle();const auto parent=tabs->parentWidget();const auto index=layout->indexOf(tabs);const auto selected=tabs->currentIndex();const auto minimum=tabs->minimumSize();const auto maximum=tabs->maximumSize();const auto nativeFont=tabs->font();
 {
 Unbound::NativeTabViewport adapter(nullptr);check(adapter.attach(tabs),"recognized native layout accepts a public scroll viewport");settle();
 auto area=row->findChild<QScrollArea *>("openGateTimelineTabViewport");auto band=surface.findChild<QWidget *>("openGateTimelineTabScrollBand");auto scroll=surface.findChild<QScrollBar *>("openGateTimelineTabScroll");

 check(area&&band&&scroll&&scroll->maximum()>100,"long tabs have a real continuous scroll model");
 check(row->height()==32&&band->geometry().top()>row->geometry().bottom()&&ruler->geometry().top()>band->geometry().bottom(),"scrollbar and padding occupy a separate row without changing native row height");
 for(int pixel=1;pixel<=25;++pixel){scroll->setValue(pixel);settle();check(area->horizontalScrollBar()->value()==pixel&&tabs->mapTo(area->viewport(),QPoint()).x()==-pixel,"single-pixel drag changes the actual viewport by one pixel");}
 check(tabs->currentIndex()==selected,"continuous scrolling preserves native selection");
 QWheelEvent wheel(QPointF(10,10),QPointF(10,10),QPoint(-3,0),QPoint(),Qt::NoButton,Qt::NoModifier,Qt::ScrollUpdate,false);QApplication::sendEvent(tabs,&wheel);settle();check(area->horizontalScrollBar()->value()==28,"trackpad pixels do not snap to tab boundaries");
 check(plus->parentWidget()==row&&plus->isVisibleTo(&surface),"native add button stays outside the scrolling tab viewport");
 const auto oldMaximum=scroll->maximum();tabs->addTab("A newly opened timeline with a very long title");settle();check(scroll->maximum()>oldMaximum,"new native tabs extend the range");
 for(int size:{10,15,20,30}){auto font=tabs->font();font.setPointSize(size);tabs->setFont(font);settle();check(band->geometry().top()>row->geometry().bottom(),"padding remains below tabs after font-scale changes");}
 tabs->setFont(nativeFont);settle();
 {Unbound::PanelMetricScale scale;
 for(double factor:{1.0,1.5,2.0,3.0}){
  check(scale.attach(&surface,factor),"full native metrics accept Timeline scale through 300 percent");settle();
  check(area->height()>=tabs->sizeHint().height() && area->height()<=row->height(),"scaled native tab text fits its viewport and native row");
  check(band->geometry().top()>row->geometry().bottom() && ruler->geometry().top()>band->geometry().bottom(),"scrollbar padding never overlaps scaled tabs or ruler");
  const int current=tabs->currentIndex();scroll->setValue(qMin(13,scroll->maximum()));settle();check(tabs->currentIndex()==current,"scaled scroll does not change selected native timeline");
 }
 scale.restore();settle();check(row->height()==32,"metric restoration returns original native row height");}
 check(adapter.restore(),"native ownership and layout slot restore");settle();
 }
 check(tabs->parentWidget()==parent&&layout->indexOf(tabs)==index&&tabs->minimumSize()==minimum&&tabs->maximumSize()==maximum&&tabs->usesScrollButtons(),"teardown restores native parent, slot, constraints and arrows");check(!surface.findChild<QScrollArea *>("openGateTimelineTabViewport")&&!surface.findChild<QWidget *>("openGateTimelineTabScrollBand"),"owned viewport and scrollbar band are removed");
 QWidget unsupported;QTabBar detached(&unsupported);Unbound::NativeTabViewport none(nullptr);check(!none.attach(&detached)&&detached.parentWidget()==&unsupported,"unrecognized ownership is left unchanged");
 std::printf("native tab viewport failures=%d\n",failures);return failures?1:0;
}
