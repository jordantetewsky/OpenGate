// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include "panellayoutsurface.h"
#include "panellayoutvisibility.h"
#include "panelscope.h"
#include <cstdio>
int main(int argc,char **argv) {
    QApplication app(argc,argv); QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen);
    auto layout=new QVBoxLayout(&main); auto split=new QSplitter(&main);layout->addWidget(split);
    auto tabs=new QTabWidget; auto pool=new QWidget; auto index=new QWidget; auto right=new QWidget;
    pool->setProperty("groupPanel","pool"); index->setProperty("groupPanel","index"); right->setProperty("groupPanel","right");
    tabs->addTab(pool,"Media Pool");tabs->addTab(index,"Index");split->addWidget(tabs);split->addWidget(right);
    main.resize(900,600);main.show();QApplication::processEvents();split->setSizes({300,600});
    Unbound::GroupVisibility state; const auto sizes=split->sizes();state.remember(split);
    Unbound::applyPanelLayoutVisibility(split,state,[](const QString &id) { return id=="right"; });state.restore(split);
    QApplication::processEvents();bool ok=tabs->isHidden() && split->sizes()[0]==0 && right->isVisible();
    state.remember(split);Unbound::applyPanelLayoutVisibility(split,state,[](const QString &) { return true; });state.restore(split);
    QApplication::processEvents();ok &= !tabs->isHidden() && split->sizes()==sizes;
    QWidget page(&main),host(&main,Qt::Tool);host.setObjectName("unboundFloating_timeline");QWidget owner(&host),mixer(&owner);
    ok &= Unbound::panelInManagedScope(&mixer,&page,&owner) && !Unbound::panelInManagedScope(&mixer,&page);
    QWidget foreign;host.setParent(&foreign,Qt::Tool);ok &= !Unbound::panelInManagedScope(&mixer,&page,&owner);host.setParent(&main,Qt::Tool);
    std::printf("modular layout recovery %s\n",ok ? "PASS" : "FAIL");return ok ? 0 : 1;
}
