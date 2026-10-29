// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <cstdio>
#include "panelsession.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
 QWidget page;page.setAttribute(Qt::WA_DontShowOnScreen);page.resize(1100,500);auto original=new QVBoxLayout(&page);
 auto timeline=new QWidget(&page);timeline->setObjectName("frameConformTimelines");auto contents=new QVBoxLayout(timeline);original->addWidget(timeline);
 auto toolbar=new QWidget(timeline);toolbar->setObjectName("frameEditToolbarTop");toolbar->setMinimumWidth(900);toolbar->setFixedHeight(32);auto tools=new QHBoxLayout(toolbar);tools->addWidget(new QPushButton("Native tools",toolbar));contents->addWidget(toolbar);
 auto tracks=new QWidget(timeline);tracks->setMinimumSize(0,150);contents->addWidget(tracks,1);
 auto nativeScroll=new QScrollBar(Qt::Horizontal,timeline);nativeScroll->setRange(0,100);nativeScroll->setValue(37);contents->addWidget(nativeScroll);
 const auto nativeMinimum=timeline->minimumSize();const auto nativePolicy=timeline->sizePolicy();
 page.show();for(int i=0;i<6;++i)app.processEvents();
 Unbound::MacPanelSession session;bool ok=session.undock(timeline,&page,"Timeline").isEmpty();
 auto host=session.window();if(!host)return 1;host->resize(480,400);for(int i=0;i<8;++i)app.processEvents();
 auto area=host->findChild<QScrollArea *>("unboundPanelOverflow_frameEditToolbarTop");
 ok &= area && area->horizontalScrollBar()->maximum()>0;
 ok &= !host->findChild<QScrollArea *>("unboundPanelOverflow_frameConformTimelines");
 ok &= timeline->minimumSize()==nativeMinimum && timeline->sizePolicy()==nativePolicy;
 auto bodyRect=tracks->geometry();if(area)area->horizontalScrollBar()->setValue(area->horizontalScrollBar()->maximum());
 for(int i=0;i<6;++i)app.processEvents();
 ok &= tracks->geometry()==bodyRect && nativeScroll->value()==37;
 ok &= session.dock().isEmpty() && toolbar->parentWidget()==timeline && contents->indexOf(toolbar)==0;
 ok &= !timeline->findChild<QScrollArea *>("unboundPanelOverflow_frameEditToolbarTop");
 std::printf("attached Timeline toolbar overflow without a body scroll wrapper %s\n",ok?"PASS":"FAIL");return ok?0:1;
}
