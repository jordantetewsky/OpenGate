// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QBoxLayout>
#include <QElapsedTimer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QToolButton>
#include <QThread>
#include <cstdio>
#include "workspacewindowadapter.h"
#include "../../../../src/core/diagnostics.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    auto settle=[] {QElapsedTimer clock;clock.start();while(clock.elapsed()<180){QApplication::processEvents();QThread::msleep(1);}};
    QWidget main;main.setObjectName("UiMainWindow");main.resize(900,600);auto layout=new QVBoxLayout(&main);
    auto strip=new QWidget(&main);strip->setObjectName("UiProjectsMainBarImp");auto titleLayout=new QHBoxLayout(strip);
    auto project=new QPushButton("Existing project title",strip);project->setObjectName("btnProjectsMainBarProjectList");titleLayout->addWidget(project);layout->addWidget(strip);
    auto toolbar=new QWidget(&main);toolbar->setObjectName("pageSingleScreenModeTools");layout->addWidget(toolbar);
    auto footer=new QWidget(&main);footer->setObjectName("m_PageSwitcherContainer");layout->addWidget(footer);
    main.show();settle();int opened=0;
    {
        QWidget page(&main);page.setGeometry(0,strip->y()+5,600,300);page.show();
        check(Unbound::nativeWorkspaceTopInset(&page)==strip->height()-5,"custom project strip overlap is reserved in logical page coordinates");
        page.move(0,strip->geometry().bottom()+1);
        check(Unbound::nativeWorkspaceTopInset(&page)==0,"native layout already below the project strip receives no duplicate inset");
        strip->hide();page.move(0,0);
        check(Unbound::nativeWorkspaceTopInset(&page)==0,"hidden project strip reserves no height");strip->show();
    }
    check(Unbound::setNativeHeaderMenu(&main,true,[&]{++opened;}),"permanent native title owner accepts Panels access");
    auto panels=strip->findChild<QToolButton *>("openGateTitlePanels");
    check(panels&&titleLayout->indexOf(panels)>=0,"Panels occupies the native title layout, with reserved space");
    toolbar->hide();footer->hide();settle();
    check(panels&&panels->isVisible(),"both hidden workspace bands leave permanent Panels access visible");
    if(panels)panels->click();check(opened==1,"native title click reaches the shared menu callback once");
    Unbound::WorkspaceHostWindow host(&main,Qt::Tool);host.resize(320,240);host.show();
    Unbound::WorkspaceHostWindow hidden(&main,Qt::Tool);hidden.resize(300,200);hidden.show();hidden.hide();settle();
    check((Unbound::nativeChromeFlags(&host)&3)==3,"managed host retains actual Windows caption and resize border");
    const auto originalOwner=GetWindow(reinterpret_cast<HWND>(host.winId()),GW_OWNER);
    check(originalOwner==reinterpret_cast<HWND>(main.winId()),"managed host retains native Resolve ownership");
    check(Unbound::setNativeBackgroundHidden(&main,{&host,&hidden},true),"native background hide succeeds with retained owned hosts");settle();
    check(!(Unbound::nativeVisibilityFlags(&main)&1)&&(Unbound::nativeVisibilityFlags(&host)&1)&&!(Unbound::nativeVisibilityFlags(&hidden)&1),"native shell hide preserves visible and user-hidden hosts independently");
    check(GetWindow(reinterpret_cast<HWND>(host.winId()),GW_OWNER)==originalOwner,"shell hide does not change native window ownership");
    check(Unbound::setNativeBackgroundHidden(&main,{&host,&hidden},false),"native background recovery succeeds");settle();
    check((Unbound::nativeVisibilityFlags(&main)&1)&&!hidden.isVisible(),"native recovery restores the main window without recalling a hidden host");
    auto stash=new QWidget(&main);stash->setObjectName("unboundNativeLayoutStash");stash->hide();
    strip->hide();strip->setParent(stash);
    check(!Unbound::setNativeHeaderMenu(&main,true,[&]{++opened;}),"parked native title slot cannot own visible workspace controls");
    check(!strip->findChild<QToolButton *>("openGateTitlePanels"),"unavailable owner leaves no orphaned Panels control");
    strip->setParent(&main);layout->insertWidget(0,strip);strip->show();
    check(Unbound::setNativeHeaderMenu(&main,true,[&]{++opened;}),"restored native title owner can reacquire Panels access");
    Unbound::setNativeHeaderMenu(&main,false,{});
    check(project->parentWidget()==strip&&project->text()=="Existing project title"&&titleLayout->indexOf(project)>=0,"accessory cleanup preserves the original title widget and layout");
    Unbound::flushDiagnostics();std::printf("Windows managed window adapter failures=%d\n",failures);return failures?1:0;
}
