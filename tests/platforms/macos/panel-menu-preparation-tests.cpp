#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <cstdio>
#include "panelmenus.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QTemporaryDir files;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    QWidget main;
    main.setAttribute(Qt::WA_DontShowOnScreen);
    QList<QWidget *> panels;
    QList<QPushButton *> buttons;
    QList<QMenu *> menus;
    Unbound::PanelMenus registry(nullptr);
    int failures=0,calls=0;
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    for(int i=0;i<12;++i) {
        auto panel=new QWidget(&main);panel->setObjectName(QString("panel%1").arg(i));panels.append(panel);
        auto button=new QPushButton(panel);button->setObjectName("btnOptions");buttons.append(button);
        auto menu=new QMenu(button);button->setMenu(menu);menus.append(menu);
        registry.add({panel->objectName(),[]{return false;},[&]{++calls;}});
        for(int j=0;j<150;++j)new QPushButton(panel);
    }
    registry.wire();
    for(int sample=0;sample<5;++sample) {
        QElapsedTimer timer;timer.start();
        for(int pass=0;pass<10;++pass)registry.wire();
        std::printf("menu_wire_ms_per_pass=%.3f\n",timer.nsecsElapsed()/10000000.0);
    }
    auto command=menus[0]->findChild<QAction *>("unboundDockPanel");
    check(command&&command->isVisible(),"parented and attached owner is usable");
    if(!command)return 1;
    buttons[1]->setMenu(menus[0]);registry.wire();
    check(!command->isVisible(),"conflicting attached owner is rejected despite known parent");
    buttons[1]->setMenu(menus[1]);registry.wire();
    check(command->isVisible(),"next pass observes detached conflict");
    buttons[1]->setMenu(menus[0]);
    QEvent show(QEvent::Show);QApplication::sendEvent(menus[0],&show);
    check(!command->isVisible(),"opening menu refreshes ownership after last wiring pass");
    buttons[1]->setMenu(menus[1]);buttons[0]->setMenu(nullptr);
    QApplication::sendEvent(menus[0],&show);
    check(command->isVisible(),"custom parented menu without attached button remains supported");
    command->trigger();QApplication::processEvents();
    check(calls==1,"repeated preparation never duplicates dispatch");
    auto late=new QMenu(buttons[2]);buttons[2]->setMenu(late);registry.wire();
    check(late->findChild<QAction *>("unboundDockPanel"),"late native menu is discovered");
    std::printf("failures=%d\n",failures);
    return failures?1:0;
}
