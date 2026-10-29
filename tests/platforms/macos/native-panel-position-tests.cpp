// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QTemporaryDir>
#include <cstdio>
#include "nativepanelposition.h"
#include "panelsession.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;
    const auto check=[&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    for (int kind=0;kind<4;++kind) {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        QWidget *owner=nullptr; QBoxLayout *box=nullptr; QGridLayout *grid=nullptr; QSplitter *split=nullptr; QStackedWidget *stack=nullptr;
        if (kind==0) { owner=new QWidget(page); box=new QHBoxLayout(owner); }
        if (kind==1) { owner=new QWidget(page); grid=new QGridLayout(owner); }
        if (kind==2) owner=split=new QSplitter(Qt::Horizontal,page);
        if (kind==3) owner=stack=new QStackedWidget(page);
        auto pageLayout=new QVBoxLayout(page); pageLayout->addWidget(owner);
        auto before=new QPushButton("Before",owner),panel=new QPushButton("Native Inspector",owner),after=new QPushButton("After",owner);
        panel->setObjectName("inspector"); panel->setMinimumWidth(130);
        if (box) { box->addWidget(before,1); box->addWidget(panel,3,Qt::AlignVCenter); box->addWidget(after,2); }
        if (grid) { grid->addWidget(before,0,0); grid->addWidget(panel,1,1,2,3,Qt::AlignVCenter); grid->addWidget(after,3,4); }
        if (split) { split->addWidget(before); split->addWidget(panel); split->addWidget(after); split->setSizes({200,500,300}); }
        if (stack) { stack->addWidget(before); stack->addWidget(panel); stack->addWidget(after); stack->setCurrentWidget(panel); }
        main.resize(1100,650); main.show(); QApplication::processEvents();
        const auto frame=main.geometry(); const auto splitSizes=split ? split->sizes() : QList<int>();
        const auto policy=panel->sizePolicy(); const auto minimum=panel->minimumSize();
        int clicks=0; QObject::connect(panel,&QPushButton::clicked,&main,[&] { ++clicks; });
        for (int cycle=0;cycle<3;++cycle) {
            Unbound::NativePanelPosition position;
            check(position.acquire(panel,page).isEmpty(),"native box/grid/split/stack panel position can be leased");
            auto marker=position.surface();
            check(marker && panel->parentWidget()==position.contentParking() && marker->parentWidget()==owner && main.geometry()==frame,
                "position marker stays in the exact native owner without changing the outer window");
            if (box) check(box->indexOf(marker)==1 && box->stretch(1)==3 && box->itemAt(1)->alignment()==Qt::AlignVCenter,
                "box stretch, order and alignment survive acquisition");
            if (grid) {
                int row,column,rowSpan,columnSpan; grid->getItemPosition(grid->indexOf(marker),&row,&column,&rowSpan,&columnSpan);
                check(row==1 && column==1 && rowSpan==2 && columnSpan==3,"grid position and spans survive acquisition");
            }
            if (split) check(split->indexOf(marker)==1 && split->sizes()==splitSizes,"native splitter index and proportions survive acquisition");
            if (stack) check(stack->indexOf(marker)==1 && stack->currentWidget()==marker,"selected native stack position survives acquisition");
            auto surface=new QWidget(&main); auto arrangement=new QHBoxLayout(surface); arrangement->addWidget(panel);
            auto extra=new QPushButton("Dropped panel",surface); arrangement->addWidget(extra);
            Unbound::NativeGroupSlot group;
            check(group.mount(marker,surface,&main).isEmpty(),"native group mounts at the leased panel position");
            QApplication::processEvents(); panel->click();
            check(clicks==cycle+1 && panel->window()==&main && extra->isVisible(),"native controls retain their signals inside the scoped arrangement");
            check(!position.restore() && position.active() && panel && extra,"occupied positions cannot be restored by deleting live group content");
            group.release(); panel->setParent(position.contentParking());
            delete surface;
            check(position.restore(),"empty scoped position restores its native panel"); QApplication::processEvents();
            check(panel->parentWidget()==owner && panel->sizePolicy()==policy && panel->minimumSize()==minimum && main.geometry()==frame,
                "restoration preserves native parent, constraints and outer frame over repeated cycles");
            if (split) check(split->sizes()==splitSizes,"native splitter proportions restore exactly");
            if (stack) check(stack->currentWidget()==panel && stack->indexOf(panel)==1,"native stack selection and index restore exactly");
        }
        {
            Unbound::NativePanelPosition position; Unbound::MacPanelSession session;
            check(position.acquire(panel,page).isEmpty() && session.undock(panel,page,"Native Inspector",true).isEmpty(),
                "a scoped native panel can enter an ordinary floating session");
            auto embedded=new QWidget(&main); auto children=new QVBoxLayout(embedded); children->addWidget(new QPushButton("Remaining group member",embedded));
            Unbound::NativeGroupSlot group;
            check(group.mount(position.surface(),embedded,&main).isEmpty(),"a second member can occupy the independent native position");
            check(!session.adoptOriginalPosition(position) && position.active() && session.active(),
                "a session cannot reclaim a native position occupied by other group members");
            group.release(); delete embedded;
            QPointer<QWidget> floating=session.window();
            check(session.adoptOriginalPosition(position) && !position.active() && session.active() && floating,
                "restoration handoff removes temporary parking without deleting the floating session");
            check(session.dock().isEmpty(),"the floating session docks through its adopted original native position");
            QApplication::processEvents();
            check(panel->parentWidget()==owner && panel->isVisible() && panel->minimumSize()==minimum && panel->sizePolicy()==policy,
                "native slot handoff restores the original panel parent, visibility and constraints");
            if (split) check(split->sizes()==splitSizes,"session handoff preserves native splitter proportions");
            if (stack) check(stack->indexOf(panel)==1 && stack->currentWidget()==panel,"session handoff preserves native stack selection");
        }
    }
    {
        QWidget main; auto page=new QWidget(&main); auto layout=new QHBoxLayout(page);
        auto panel=new QPushButton("Lifetime",page); layout->addWidget(panel); page->show(); main.show(); QApplication::processEvents();
        QPointer<QWidget> original=panel; Unbound::NativePanelPosition position;
        check(position.acquire(panel,page).isEmpty(),"position lifetime preparation");
        QPointer<QWidget> marker=position.surface(); delete page;
        check(!original && !marker && !position.active() && position.restore(),"page destruction owns both original and leased native trees");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto layout=new QHBoxLayout(page); QPointer<QWidget> panel=new QPushButton("Rebound lifetime",page); layout->addWidget(panel);
        main.show(); QApplication::processEvents();
        Unbound::NativePanelPosition position; Unbound::MacPanelSession session;
        check(position.acquire(panel,page).isEmpty() && session.undock(panel,page,"Rebound",true).isEmpty() && session.adoptOriginalPosition(position),
            "native position lifetime can transfer to the session");
        QPointer<QWidget> floating=session.window(); delete page;
        QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        check(!session.active() && !floating && !panel,"the adopted native parent owns session cleanup after parking is deleted");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto layout=new QHBoxLayout(page); auto panel=new QPushButton("Foreign replacement",page); layout->addWidget(panel);
        main.show(); QApplication::processEvents(); Unbound::NativePanelPosition position;
        check(position.acquire(panel,page).isEmpty(),"foreign position replacement preparation");
        auto marker=position.surface(); auto foreign=new QPushButton("Native replacement",page);
        auto old=layout->replaceWidget(marker,foreign); delete old;
        check(!position.restore() && position.active() && panel->parentWidget()==position.contentParking() && layout->indexOf(foreign)>=0,
            "foreign native layout edits retain both trees rather than deleting or overwriting controls");
        old=layout->replaceWidget(foreign,marker); delete old; delete foreign;
        check(position.restore() && layout->indexOf(panel)>=0,"position restoration retries after native ownership is repaired");
    }
    {
        QWidget main,page(&main),unmanaged(&page); main.show(); page.show(); unmanaged.show();
        Unbound::NativePanelPosition position;
        check(!position.acquire(&unmanaged,&page).isEmpty() && unmanaged.parentWidget()==&page && !position.active(),
            "unsupported unmanaged native positions are rejected without reparenting");
    }
    if (!failures) std::puts("PASS: scoped native box/grid/split/stack positions, group content, recovery and lifetime");
    return failures ? 1 : 0;
}
