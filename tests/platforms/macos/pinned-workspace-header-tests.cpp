// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QElapsedTimer>
#include <QThread>
#include <QTemporaryDir>
#include <cstdio>
#include "pinnedworkspaceheader.h"
#include "workspaceheadermigration.h"
int main(int argc,char **argv){
 QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
 int errors=0;auto check=[&](bool ok,const char *message){if(!ok){++errors;std::fprintf(stderr,"FAIL: %s\n",message);}};
 auto settle=[] {QElapsedTimer t;t.start();while(t.elapsed()<150){QApplication::processEvents();QThread::msleep(1);}};
 const auto oldTree=Unbound::splitGroupTree({{"panel","unboundWorkspaceToolbar"}},{{"panel","timeline"}},Qt::Vertical);
 QJsonObject oldGroup{{"tree",oldTree},{"panels",QJsonArray{"unboundWorkspaceToolbar","timeline"}},{"x",40},{"y",80},{"width",900},{"height",600}};
 auto migrated=Unbound::migrateWorkspaceHeader({{"custom-field",42},{"footer",QJsonObject{{"surfaces",QJsonObject{{"unboundFloating_group_timeline_unboundWorkspaceToolbar",false}}}}},{"groups",QJsonArray{oldGroup}},{"panels",QJsonObject{{"unboundWorkspaceToolbar",QJsonObject{{"floating",true},{"hidden",true}}},{"timeline",QJsonObject{{"pinned",true}}}}}});
 check(migrated["footer"].toObject()["surfaces"].toObject().contains("unboundFloating_timeline")&&!migrated["footer"].toObject()["surfaces"].toObject()["unboundFloating_timeline"].toBool(),"footer unpin choice follows a renamed legacy surface");
 check(migrated["custom-field"]==42&&migrated["groups"].toArray().isEmpty(),"legacy toolbar group migrates without discarding unknown settings");
 check(!migrated["header"].toObject()["visible"].toBool()&&migrated["panels"].toObject()["timeline"].toObject()["pinned"].toBool()&&migrated["panels"].toObject()["timeline"].toObject()["width"]==900,"migration retains hidden header choice and remaining surface frame");
 auto attached=Unbound::migrateWorkspaceHeader({{"groups",QJsonArray{QJsonObject{{"tree",Unbound::splitGroupTree({{"panel","frameEditToolbarTop"}},{{"panel","frameConformTimelines"}},Qt::Vertical)},{"width",900},{"height",600}}}}});
 check(attached["groups"].toArray().isEmpty()&&!attached["panels"].toObject()["frameEditToolbarTop"].toObject()["floating"].toBool()&&attached["panels"].toObject()["frameConformTimelines"].toObject()["width"]==900,"legacy separate Timeline toolbar rejoins native Timeline without losing remaining group geometry");
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);auto outer=new QVBoxLayout(&main);auto page=new QWidget(&main);outer->addWidget(page);auto native=outer;
 auto toolbar=new QWidget(&main);toolbar->setObjectName("NativeToolbar");auto tools=new QHBoxLayout(toolbar);auto button=new QPushButton("Native action",toolbar);tools->addWidget(button);native->insertWidget(0,toolbar);(new QVBoxLayout(page))->addWidget(new QWidget(page));
 QWidget first(&main,Qt::Tool),second(&main,Qt::Tool);for(auto host:{&first,&second}){host->setAttribute(Qt::WA_DontShowOnScreen);host->resize(600,400);auto box=new QVBoxLayout(host);box->addWidget(new QWidget(host));host->show();}
 main.resize(900,600);main.show();settle();auto originalParent=toolbar->parentWidget();const auto index=native->indexOf(toolbar);const auto minimum=toolbar->minimumSize();const auto policy=toolbar->sizePolicy();QWidget *primary=&first;int clicks=0;QObject::connect(button,&QPushButton::clicked,[&]{++clicks;});
 {
 int discoveries=0;Unbound::PinnedWorkspaceHeader startup(nullptr);
 startup.page=[&]() -> QWidget * {++discoveries;return nullptr;};
 QWidget child(&main);
 for(int i=0;i<1000;++i){QEvent event(QEvent::ParentChange);QApplication::sendEvent(&child,&event);}
 check(discoveries==0,"native construction events never trigger recursive page discovery in header filter");
 }
 {
 Unbound::PinnedWorkspaceHeader header(nullptr);header.page=[&]{return page;};header.toolbar=[&]{return toolbar;};header.primary=[&]{return primary;};header.refresh();settle();
 auto band=first.findChild<QScrollArea *>("unboundAttachedAccessory_NativeToolbar");auto layout=qobject_cast<QVBoxLayout *>(first.layout());
 check(band&&layout->indexOf(band)==0,"real toolbar is reserved above all surface content");
 check(toolbar->window()==&first&&toolbar->findChild<QPushButton *>()==button,"native controls retain identity and signal wiring");button->click();check(clicks==1,"original native action executes exactly once");
 auto body=layout->itemAt(1)->widget();check(body->geometry().top()>band->geometry().bottom(),"content never overlaps pinned header");check(!header.externalBounds(&second).isEmpty() && header.externalBounds(&first).isEmpty(),"only external expanded hosts receive a header gate");
 int height=body->height();header.setShown(false);settle();check(band->isHidden()&&body->height()>height,"hide reclaims reserved top space");check(header.externalBounds(&second).isEmpty(),"hidden header removes maximize gate");
 QMenu recovery;header.populateMenu(&recovery);check(!recovery.actions().first()->isChecked(),"Panels visibility reflects the one state model");recovery.actions().first()->trigger();settle();check(!band->isHidden(),"Panels restores hidden header");
 primary=&second;header.refresh();settle();check(toolbar->window()==&second&&!first.findChild<QScrollArea *>("unboundAttachedAccessory_NativeToolbar"),"surface transfer removes previous band without duplicating native content");
 page->hide();settle();check(toolbar->parentWidget()==originalParent&&native->indexOf(toolbar)==index,"page departure restores original toolbar ownership");
 page->show();settle();check(toolbar->window()==&second,"page return remounts header");
 primary=&main;header.refresh();settle();check(toolbar->parentWidget()==originalParent&&native->indexOf(toolbar)==index,"explicit native return restores exact original slot");
 check(toolbar->minimumSize()==minimum&&toolbar->sizePolicy()==policy,"native constraints and policy survive transfer");
 }
 check(toolbar->parentWidget()==originalParent,"shutdown preserves native controls");
 std::printf("pinned header failures=%d\n",errors);return errors?1:0;
}
