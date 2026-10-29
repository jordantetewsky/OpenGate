// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "timelinecontrol.h"
int main(int argc,char **argv) {
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1");
    QApplication app(argc,argv); QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget main; main.setObjectName("UiMainWindow"); main.setAttribute(Qt::WA_DontShowOnScreen);
    auto outer=new QVBoxLayout(&main); auto timeline=new QWidget(&main); timeline->setObjectName("frameConformTimelines"); outer->addWidget(timeline);
    auto layout=new QVBoxLayout(timeline); auto toolbar=new QWidget(timeline); toolbar->setObjectName("frameEditToolbarTop"); layout->addWidget(toolbar);
    auto content=new QLabel("Timeline content",timeline); layout->addWidget(content);
    int calls=0; QObject owner;
    {
        Unbound::TimelineControl controls(&owner); controls.wire(timeline,[&] { ++calls; },[](QPoint) {});
        auto button=timeline->findChild<Unbound::NativeUndockButton *>("unboundTimelineUndock");
        check(button && button->drag,"Timeline has its own click/drag control");
        QWidget floatingTools; toolbar->setParent(&floatingTools);
        check(button && timeline->isAncestorOf(button),"toolbar separation cannot take Timeline control away");
        if (button) button->click(); QApplication::processEvents(); check(calls==1,"click dispatches once after native event closes");
        controls.wire(timeline,[&] { calls+=10; },[](QPoint) {});
        if (button) button->click(); QApplication::processEvents(); check(calls==11,"rewiring updates callback without duplicate connections");
        layout->insertWidget(1,toolbar); check(timeline->isAncestorOf(toolbar),"reattached toolbar remains inside Timeline transfer subtree");
    }
    check(!timeline->findChild<QWidget *>("unboundTimelineHeader"),"controller teardown removes its own chrome");
    {
        QWidget alternate; auto horizontal=new QHBoxLayout(&alternate); horizontal->addWidget(new QLabel("Native content",&alternate));
        {
            Unbound::TimelineControl controls(nullptr); controls.wire(&alternate,[] {},[](QPoint) {});
            check(alternate.findChild<QToolButton *>("unboundTimelineUndock") && horizontal->direction()==QBoxLayout::TopToBottom,
                "single-content native horizontal container can expose the Timeline control");
        }
        check(horizontal->direction()==QBoxLayout::LeftToRight && horizontal->count()==1,
            "native layout direction and content count restore exactly on teardown");
    }
    if (!failures) std::puts("PASS: independent Timeline control, toolbar attachment, click dispatch and teardown");
    return failures ? 1 : 0;
}
