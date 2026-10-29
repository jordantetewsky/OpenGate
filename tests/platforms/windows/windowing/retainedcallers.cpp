// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QLibrary>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>
#include <cstdio>
#include <windows.h>
#include "../../../../src/core/panesettings.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QDir().mkpath("build/windows/fixtures");
    QTemporaryDir directory(QDir::currentPath()+"/build/windows/fixtures/retained-callers-XXXXXX");
    directory.setAutoRemove(false);
    if(!directory.isValid())return 2;
    std::printf("Retained-caller evidence: %s\n",qPrintable(directory.path()));
    qputenv("UNBOUND_SETTINGS_FILE",directory.filePath("Unbound.json").toUtf8());
    qputenv("UNBOUND_DIAGNOSTICS_DIR",directory.filePath("Logs").toUtf8());
    qputenv("OPENGATE_WINDOWS_MANAGED_CALLERS","1");
    int failures=0;auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    auto turn=[&]{for(int i=0;i<8;++i)app.processEvents();};
    auto click=[&](QWidget *widget){
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(4,4),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(4,4),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(widget,&press);QApplication::sendEvent(widget,&release);turn();
    };
    QMainWindow main;main.setObjectName("UiMainWindow");main.resize(1000,700);main.menuBar()->addMenu("Workspace");
    auto page=new QWidget(&main);page->setObjectName("WStackPage_Conform");main.setCentralWidget(page);
    auto pageLayout=new QVBoxLayout(page);
    auto widgets=new QWidget(page);widgets->setObjectName("frameEditPageWidgetsContainer");pageLayout->addWidget(widgets);
    auto column=new QVBoxLayout(widgets);
    auto toggle=new QPushButton("Inspector",page);toggle->setObjectName("btnEditToggleInspectorInSSM");toggle->setCheckable(true);toggle->setChecked(true);column->addWidget(toggle);
    auto pane=new QWidget(page);pane->setObjectName("frameEditPageInspector");column->addWidget(pane);
    auto original=new QVBoxLayout(pane);auto live=new QPushButton("Native content",pane);original->addWidget(live);
    int nativeToggleCalls=0,edits=0;
    QObject::connect(toggle,&QPushButton::clicked,page,[&](bool shown){++nativeToggleCalls;pane->setVisible(shown);});
    QObject::connect(live,&QPushButton::clicked,page,[&]{++edits;});
    main.show();turn();
    QLibrary helper(app.arguments().last());helper.setLoadHints(QLibrary::PreventUnloadHint);
    check(helper.load(),qPrintable(helper.errorString()));
    using Start=DWORD(WINAPI *)(void *);auto start=reinterpret_cast<Start>(helper.resolve("UnboundStart"));
    check(start&&start(nullptr)==1,"production Windows bootstrap queues controller");turn();
    auto button=pane->findChild<QToolButton *>("resolvePaneUndockButton");
    check(button!=nullptr,"production decoration discovered");if(button)button->click();turn();
    auto host=live->window();check(host!=&main&&host->layout()==original,"real production session leases native layout");
    if(host==&main)return 1;
    host->setGeometry(150,160,420,340);turn();const auto frame=host->geometry();const auto owner=live->parentWidget();
    check(toggle->isChecked()&&pane->isHidden(),"header reflects visible retained host while native vacancy stays closed");
    pane->show();turn();check(pane->isHidden()&&host->isVisible(),"native visibility updates cannot reopen an empty leased slot");
    const int before=nativeToggleCalls;click(toggle);
    check(host->isHidden()&&!toggle->isChecked()&&live->parentWidget()==owner,"native header hides the same leased host");
    check(nativeToggleCalls==before,"managed header input does not invoke native empty-slot toggle");
    check(Unbound::PaneSettings().hidden("frameEditPageInspector"),"hidden working state persisted");
    page->hide();turn();page->show();turn();check(host->isHidden(),"page return cannot reveal a user-hidden host");
    auto menu=main.findChild<QMenu *>("openGatePanels");check(menu!=nullptr,"flat Panels menu reachable in native menu bar");
    if(menu){QMetaObject::invokeMethod(menu,"aboutToShow");QAction *inspector=nullptr;
        for(auto action:menu->actions()){
            check(!action->menu(),"current-page inventory is flat");
            if(action->objectName()=="openGatePanelToggle_frameEditPageInspector")inspector=action;
        }
        check(inspector&&inspector->isEnabled()&&!inspector->isChecked(),"retained hidden Inspector is an available unchecked caller");
        if(inspector)inspector->trigger();turn();}
    check(host->isVisible()&&live->window()==host&&host->geometry()==frame&&live->parentWidget()==owner,"Panels recall retains exact host, geometry and content owner");
    check(toggle->isChecked()&&!Unbound::PaneSettings().hidden("frameEditPageInspector"),"menu and header share persisted visibility");
    live->click();check(edits==1,"native signal connection survives hide/recall");
    page->hide();turn();check(host->isHidden(),"page departure hides visible retained host");page->show();turn();
    check(host->isVisible()&&host->geometry()==frame,"visible host returns to remembered location");
    if(button)button->click();turn();
    check(pane->layout()==original&&live->parentWidget()==pane&&pane->isVisible(),"explicit dock restores exact native layout, owner and visibility");
    check(!Unbound::PaneSettings().floating("frameEditPageInspector"),"explicit ownership return persists docking intent");
    // Exercise ordinary shutdown: the production controller and journal own it.
    QTimer::singleShot(0,&app,&QCoreApplication::quit);app.exec();
    std::printf("Windows production retained-caller failures=%d\n",failures);return failures?1:0;
}
