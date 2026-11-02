// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QWheelEvent>
#include <cstdio>
#include "../../../src/core/menuoverflowscroll.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv);int failures=0;
    auto check=[&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    auto settle=[]{for(int i=0;i<12;++i)QApplication::processEvents();};
    QMenu menu;menu.setAttribute(Qt::WA_DontShowOnScreen);menu.setMaximumHeight(320);
    auto font=menu.font();font.setPixelSize(30);menu.setFont(font);
    int calls=0;for(int i=0;i<100;++i) {
        auto action=menu.addAction(QString("Native color %1").arg(i));
        QObject::connect(action,&QAction::triggered,&menu,[&]{++calls;});
    }
    auto last=menu.actions().back();menu.popup(QPoint(200,200));settle();
    Unbound::MenuOverflowScroll::refresh(&menu);settle();
    auto bar=menu.findChild<QScrollBar *>("openGateMenuOverflowScroll");
    check(bar && bar->isVisible() && bar->maximum()>0,"overflowing native menu exposes a real visible scrollbar");
    check(menu.screen()->availableGeometry().contains(menu.frameGeometry()),"overflowing menu stays inside its screen");
    if(bar && menu.actionGeometry(last).right()>=bar->x())std::fprintf(stderr,"menu width=%d action right=%d scrollbar x=%d\n",menu.width(),menu.actionGeometry(last).right(),bar->x());
    check(bar && menu.actionGeometry(last).right()<bar->x(),"scrollbar reserves space beside native labels/icons");
    if(bar){bar->setValue(bar->maximum());settle();}
    check(menu.rect().contains(menu.actionGeometry(last)) && calls==0,"scrollbar reaches the last native item without executing any action");
    const auto point=menu.actionGeometry(last).center(),global=menu.mapToGlobal(point);
    QMouseEvent down(QEvent::MouseButtonPress,point,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&menu,&down);
    QMouseEvent up(QEvent::MouseButtonRelease,point,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(&menu,&up);settle();
    check(calls==1,"scrolled native item still executes exactly once through its original action");
    menu.popup(QPoint(200,200));settle();Unbound::MenuOverflowScroll::refresh(&menu);settle();
    if(bar){bar->setValue(bar->minimum());settle();}
    check(menu.actionGeometry(menu.actions().front()).top()>=0,"scrollbar can return to the first item after reopening");
    menu.hide();
    QMenu late;late.setAttribute(Qt::WA_DontShowOnScreen);late.addAction("Native color");late.popup(QPoint(200,200));settle();
    late.addAction("Preview Red (Temporary)");late.addAction("Clear Temporary Clip Color Preview");
    const auto bounds=late.screen()->availableGeometry();late.move(bounds.right()-20,bounds.bottom()-20);
    Unbound::MenuOverflowScroll::refresh(&late);settle();
    check(bounds.contains(late.frameGeometry()),"late inserted actions refit a popup near the bottom/right screen edge");
    auto shortBar=late.findChild<QScrollBar *>("openGateMenuOverflowScroll");
    check(shortBar && !shortBar->isVisible(),"fitting menus do not waste space on a scrollbar");
    std::printf("menu overflow failures=%d\n",failures);return failures?1:0;
}
