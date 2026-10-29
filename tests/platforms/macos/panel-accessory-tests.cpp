// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QTemporaryDir>
#include <cstdio>
#include "footerpanel.h"
#include "panelaccessorylease.h"
#include "panelsession.h"

class ParentlessNativeGuard : public QObject {
public:
    int parentless=0;
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::ParentChange)if(auto widget=qobject_cast<QWidget *>(object))
            if(!widget->parentWidget())++parentless;
        return false;
    }
};
int main(int argc,char **argv)
{
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1");
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;
    const auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen); auto native=new QVBoxLayout(&main);
    auto timeline=new QWidget(&main); timeline->setObjectName("frameConformTimelines");
    auto content=new QVBoxLayout(timeline); content->addWidget(new QPushButton("Timeline content",timeline)); native->addWidget(timeline);
    QPointer<QWidget> footer=new QWidget(&main); footer->setObjectName("m_PageSwitcherContainer");
    footer->setMinimumWidth(800); footer->setFixedHeight(32); auto row=new QHBoxLayout(footer);
    row->addWidget(new QPushButton("Native page action",footer)); native->addWidget(footer);
    main.resize(1000,500); main.show(); QApplication::processEvents();
    const auto policy=footer->sizePolicy(); const auto fill=footer->autoFillBackground();
    {
        Unbound::PanelAccessoryLease lease(nullptr);
        check(lease.attach(footer,&main,timeline).isEmpty(),"native footer attaches to owner without a floating window");
        QWidget carrier; carrier.setAttribute(Qt::WA_DontShowOnScreen); auto carried=new QVBoxLayout(&carrier);
        carried->addWidget(timeline); carrier.resize(400,350); carrier.show(); QApplication::processEvents();
        check(footer && footer->window()==&carrier && lease.active(),"moving owner carries the actual native footer through Qt ownership");
        auto viewport=timeline->findChild<QScrollArea *>("unboundAttachedAccessory_m_PageSwitcherContainer");
        check(viewport && viewport->horizontalScrollBar()->maximum()>0 && carrier.width()==400,
            "wide native footer remains reachable without forcing owner width");
        lease.setHidden(true); check(lease.hidden(),"footer door collapses its attached surface");
        lease.setHidden(false); check(!lease.hidden(),"footer door reopens without detaching");
        check(lease.release() && footer->parentWidget()==&main && native->indexOf(footer)==0 && native->count()==1,
            "release restores exact original native layout slot");
        check(footer->minimumWidth()==800 && footer->sizePolicy()==policy && footer->autoFillBackground()==fill,
            "release restores native metrics and background behavior");
        native->insertWidget(0,timeline);
        auto unsupported=new QWidget(&main); auto horizontal=new QHBoxLayout(unsupported); horizontal->addStretch();
        check(!lease.attach(footer,&main,unsupported).isEmpty() && footer->parentWidget()==&main,
            "unsupported attachment layout leaves native content untouched"); delete unsupported;
        check(lease.attach(footer,&main,timeline).isEmpty(),"accessory can attach again after restoration");
        QWidget foreign(&main); footer->setParent(&foreign);
        check(!lease.release() && footer->parentWidget()==&foreign,
            "unexpected ownership rejects restoration without stealing foreign content");
        viewport=timeline->findChild<QScrollArea *>("unboundAttachedAccessory_m_PageSwitcherContainer");
        viewport->takeWidget(); viewport->setWidget(footer); footer->show();
        delete timeline; timeline=nullptr;
        check(footer && footer->parentWidget()==&main && !lease.active(),"owner destruction restores footer instead of deleting native content");
    }
    {
        Unbound::FooterPanel panel(nullptr,[&] {return &main;},{{"floating",true},{"attachment","timeline"}});
        panel.restore();QApplication::processEvents();
        check(footer->parentWidget()==&main&&!panel.snapshot()["floating"].toBool(),"legacy floating footer state migrates without moving native content");
        check(panel.hidePanel().isEmpty()&&footer->isHidden(),"global hide retains native slot");
        check(panel.showPanel().isEmpty()&&footer->isVisible(),"global show restores native visibility");
        panel.shutdown();check(footer->parentWidget()==&main,"shutdown releases presentation without a native lease");
    }
    Unbound::flushDiagnostics();
    {
        QWidget root;root.setObjectName("UiMainWindow");root.setAttribute(Qt::WA_DontShowOnScreen);
        auto outer=new QVBoxLayout(&root);
        auto page=new QWidget(&root);page->setObjectName("WStackPage_Conform");outer->addWidget(page);
        auto pageLayout=new QVBoxLayout(page);
        auto panel=new QWidget(page);panel->setObjectName("frameConformTimelines");
        auto panelLayout=new QVBoxLayout(panel);panelLayout->addWidget(new QPushButton("Native timeline",panel));pageLayout->addWidget(panel);
        auto tools=new QWidget(&root);tools->setObjectName("NativeWorkspaceTools");tools->setFixedHeight(32);outer->addWidget(tools);
        auto pages=new QWidget(&root);pages->setObjectName("NativePageNavigation");pages->setFixedHeight(32);outer->addWidget(pages);
        ParentlessNativeGuard ownershipGuard;tools->installEventFilter(&ownershipGuard);pages->installEventFilter(&ownershipGuard);
        root.resize(800,600);root.show();QApplication::processEvents();
        Unbound::MacPanelSession session(nullptr);
        check(session.undock(panel,page,"Timeline",false,true).isEmpty(),"prepare a hidden standalone panel host");
        auto originalHost=session.window();originalHost->setAttribute(Qt::WA_DontShowOnScreen);
        Unbound::PanelAccessoryLease top(nullptr),bottom(nullptr);
        check(top.attach(tools,&root,originalHost,Qt::TopEdge).isEmpty() && bottom.attach(pages,&root,originalHost).isEmpty(),"reserve both workspace bands before grouping");
        QWidget group(&root,Qt::Tool);group.setAttribute(Qt::WA_DontShowOnScreen);
        auto groupLayout=new QVBoxLayout(&group);auto cell=new QWidget(&group);groupLayout->addWidget(cell);
        int releases=0;session.beforeLayoutTransfer=[&]() -> QString {
            ++releases;
            return top.release() && bottom.release() ? QString() : QString("band release failed");
        };
        check(session.attachToGroup(cell,true).isEmpty(),"grouping releases workspace bands before donating its layout");
        check(releases==1 && tools->parentWidget()==&root && pages->parentWidget()==&root &&
              !cell->isAncestorOf(tools) && !cell->isAncestorOf(pages),"group leaf contains panel content only, never workspace bands");
        check(top.attach(tools,&root,&group,Qt::TopEdge).isEmpty() && bottom.attach(pages,&root,&group).isEmpty(),"bands mount at the outer group surface after transfer");
        check(top.release() && bottom.release(),"both bands restore without ownership rejection after grouping");
        session.detachFromGroup(true);check(session.dock().isEmpty(),"Native return restores the original timeline after band transfer");
        check(panel->parentWidget()==page && pageLayout->indexOf(panel)>=0,"native panel slot survives grouping and return");
        check(ownershipGuard.parentless==0,"native workspace controls never become temporary top-level windows during release");
        tools->removeEventFilter(&ownershipGuard);pages->removeEventFilter(&ownershipGuard);
        session.beforeLayoutTransfer={};
    }
    if (!failures) std::puts("PASS: accessory ownership, overflow, native restoration, owner destruction and footer attachment state");
    return failures ? 1 : 0;
}
