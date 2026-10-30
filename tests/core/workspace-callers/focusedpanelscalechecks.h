// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/focusedpanelscalecommands.h"
#include <QPushButton>
#include <QVBoxLayout>
namespace OpenGateTest {
template<class Check> void focusedPanelScaleChecks(Check check) {
    qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
    QWidget page;page.setAttribute(Qt::WA_DontShowOnScreen);page.resize(700,500);auto outer=new QVBoxLayout(&page);
    QWidget inspector,pool;inspector.setObjectName("widgetEditInspector");pool.setObjectName("frameEditPageMediaPool");
    auto firstLayout=new QVBoxLayout(&inspector);auto first=new QPushButton("Inspector control");first->setFixedSize(100,40);firstLayout->addWidget(first);
    auto secondLayout=new QVBoxLayout(&pool);auto second=new QPushButton("Media Pool control");second->setFixedSize(100,40);secondLayout->addWidget(second);
    outer->addWidget(&inspector);outer->addWidget(&pool);page.show();QApplication::processEvents();
    Unbound::ManagedPanelSession firstSession,secondSession;
    check(firstSession.undock(&inspector,&page,"Inspector").isEmpty()&&secondSession.undock(&pool,&page,"Media Pool").isEmpty(),"focused scale uses actual managed sessions");
    int firstSaves=0,secondSaves=0;
    firstSession.arrangementChanged=[&]{++firstSaves;};secondSession.arrangementChanged=[&]{++secondSaves;};
    Unbound::PanelCommands commands(nullptr);commands.context=[]{return true;};
    commands.add({"panel.toggle.widgetEditInspector","Inspector",[]{return true;},[]{}});
    commands.add({"panel.toggle.frameEditPageMediaPool","Media Pool",[]{return true;},[]{}});
    Unbound::FocusedPanelScaleCommands::addTo(&commands,[&](const QString &id)->Unbound::ManagedPanelSession * {
        return id=="widgetEditInspector"?&firstSession:id=="frameEditPageMediaPool"?&secondSession:nullptr;
    });
    check(commands.save({{"panel.scale.increase","F17"},{"panel.scale.decrease","F18"},{"panel.scale.reset","F19"}}).isEmpty(),"one shared scale key set saves");
    auto send=[&](QWidget *receiver,int code) {
        QApplication::setActiveWindow(receiver->window());receiver->setFocus();QApplication::processEvents();
        QKeyEvent override(QEvent::ShortcutOverride,code,Qt::NoModifier);QApplication::sendEvent(receiver,&override);
        QKeyEvent press(QEvent::KeyPress,code,Qt::NoModifier);QApplication::sendEvent(receiver,&press);
        for(int i=0;i<4;++i)QApplication::processEvents();
    };
    send(first,Qt::Key_F17);
    check(qFuzzyCompare(firstSession.interfaceScale(),1.1)&&secondSession.interfaceScale()==1.&&first->minimumSize()==QSize(110,44),"increase changes only focused Inspector content");
    send(second,Qt::Key_F18);
    check(qFuzzyCompare(secondSession.interfaceScale(),0.9)&&qFuzzyCompare(firstSession.interfaceScale(),1.1)&&second->minimumSize()==QSize(90,36),"same scale command set targets focused Media Pool independently");
    check(firstSaves==1&&secondSaves==1,"scale commands notify existing persistence path once per change");
    send(first,Qt::Key_F19);check(firstSession.interfaceScale()==1.&&qFuzzyCompare(secondSession.interfaceScale(),0.9),"reset affects only focused panel");
    check(secondSession.dock().isEmpty()&&second->minimumSize()==QSize(100,40)&&qFuzzyCompare(secondSession.interfaceScale(),0.9),"native restoration retains preference and restores original metrics");
    check(secondSession.undock(&pool,&page,"Media Pool").isEmpty()&&second->minimumSize()==QSize(90,36),"re-entry reapplies the same retained scale value");
}
}
