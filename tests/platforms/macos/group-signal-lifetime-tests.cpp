// SPDX-License-Identifier: MIT
#include <QAccessible>
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "panelgroup.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;
    const auto check=[&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
    auto row=new QHBoxLayout(page); QList<QWidget *> controls;
    for (int i=0;i<4;++i) { auto widget=new QWidget(page); widget->setObjectName(QString("lifetime%1").arg(i)); row->addWidget(widget); controls.append(widget); }
    main.show(); QApplication::processEvents(); Unbound::MacPanelSession a,b,c,d;
    check(a.undock(controls[0],page,"A").isEmpty() && b.undock(controls[1],page,"B").isEmpty() &&
        c.undock(controls[2],page,"C").isEmpty() && d.undock(controls[3],page,"D").isEmpty(),"signal lifetime panels detach");
    Unbound::PanelGroup left(nullptr),right(nullptr);
    check(left.join(&a,&b,Qt::Horizontal).isEmpty() && right.join(&c,&d,Qt::Vertical).isEmpty(),"independent groups are available for transfer");
    QPointer<QToolButton> button=right.window()->findChild<QToolButton *>("unboundDetachPanel");
    check(button,"transferred member owns its original placement button");
    QObject observer; int clicked=0,destroyed=0;
    QObject::connect(button,&QToolButton::clicked,&observer,[&] { ++clicked; });
    QObject::connect(button,&QObject::destroyed,&observer,[&] { ++destroyed; });
    auto interface=QAccessible::queryAccessibleInterface(button); const auto id=interface ? QAccessible::uniqueId(interface) : 0;
    check(id!=0,"Qt accessibility tracks the native button before transfer");
    check(left.merge(&right,Qt::Horizontal).isEmpty(),"multi-panel group transfer succeeds");
    if (button) button->click();
    check(clicked==1,"group transfer preserves unrelated button signal receivers");
    QApplication::processEvents();
    check(!left.contains(&c) || !left.contains(&d),"transferred placement action detaches from its new group");
    left.dissolve(); QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    check(!button && destroyed==1,"group teardown preserves external destruction notifications");
    check(!id || !QAccessible::accessibleInterface(id),"destroyed group buttons are removed from Qt accessibility's interface cache");
    // Clean a failed fixture's stale cache entry before Qt application teardown.
    // This does not run in the product or replace either assertion above.
    if (id && QAccessible::accessibleInterface(id)) QAccessible::deleteAccessibleInterface(id);
    if (!failures) std::puts("PASS: group transfer preserves external signals and Qt accessibility lifetime tracking");
    return failures ? 1 : 0;
}
