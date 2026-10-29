// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QFile>
#include <QElapsedTimer>
#include <QThread>
#include <QPushButton>
#include <QTemporaryDir>
#include <cstdio>
#include "nativegroupslot.h"
#include "nativerootlayout.h"
#include "panellayoutsurface.h"
#include "panelsession.h"
#include "nativearrangement.h"
#include "nativechrome.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir files; qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;
    const auto check=[&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    {
        QWidget main; auto layout=new QVBoxLayout(&main);
        auto slot=new Unbound::NativePanelSlot(&main); slot->setObjectName("unboundVacatedSlot_fixture"); layout->addWidget(slot); slot->hide();
        auto surface=new QWidget(&main); auto controls=new QHBoxLayout(surface); auto button=new QPushButton("Native control",surface); controls->addWidget(button);
        const auto minimum=slot->minimumSize(); const auto policy=slot->sizePolicy(); const bool hidden=slot->isHidden();
        main.resize(700,400); main.show(); QApplication::processEvents();
        const auto key=Unbound::nativeKeyWindowNumber();
        Unbound::NativeGroupSlot mount;
        check(mount.mount(slot,surface,&main).isEmpty(),"owned native layout slot accepts an embedded arrangement"); QApplication::processEvents();
        check(surface->window()==&main && !surface->isWindow() && slot->isVisible() && button->isVisible(),
            "embedded content remains real controls inside the original native window");
        check(Unbound::nativeKeyWindowNumber()==key,"embedding does not activate another native window");
        check(slot->minimumSizeHint().width()>=button->minimumSizeHint().width(),"embedded arrangement supplies the native slot's usable size floor");
        mount.setVisible(false); check(slot->isHidden(),"all-hidden embedded group can collapse its native slot");
        mount.setVisible(true); check(!slot->isHidden(),"revealing an embedded group returns its same native slot");
        mount.release();
        check(!slot->layout() && slot->minimumSize()==minimum && slot->sizePolicy()==policy && slot->isHidden()==hidden,
            "unmount restores the exact empty-slot constraints and visibility");
        check(button && surface->parentWidget()==&main && !surface->isWindow(),"unmount never deletes or floats native content implicitly");
    }
    {
        QWidget main; QWidget nativePanel(&main),surface(&main);
        Unbound::NativeGroupSlot mount;
        check(!mount.mount(&nativePanel,&surface,&main).isEmpty() && !nativePanel.layout() && surface.parentWidget()==&main,
            "unrecognized native panels cannot be overwritten as restoration slots");
        Unbound::NativePanelSlot slot(&main); auto existing=new QHBoxLayout(&slot);
        check(!mount.mount(&slot,&surface,&main).isEmpty() && slot.layout()==existing,
            "existing native layouts are rejected without ownership changes");
        check(!mount.mount(&slot,&slot,&main).isEmpty(),"cyclic native placement is rejected");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto original=new QHBoxLayout(page); auto nativeControl=new QPushButton("Native original",page); original->addWidget(nativeControl);
        auto unmanaged=new QPushButton("Native unmanaged",page); unmanaged->setGeometry(0,0,50,20);
        auto custom=new QWidget(&main); auto controls=new QVBoxLayout(custom); auto arranged=new QPushButton("Arranged native control",custom); controls->addWidget(arranged);
        page->setMinimumSize(500,200); const auto minimum=page->minimumSize(); const auto policy=page->sizePolicy();
        main.show(); QApplication::processEvents(); const bool unmanagedHidden=unmanaged->isHidden();
        const auto key=Unbound::nativeKeyWindowNumber(); Unbound::NativeRootLayout lease;
        check(lease.mount(page,custom).isEmpty(),"native page layout can be leased without replacing its main window"); QApplication::processEvents();
        check(page->layout()!=original && !nativeControl->isVisible() && !unmanaged->isVisible() && arranged->isVisible(),
            "custom arrangement occupies one workspace without displaying its parked native layout behind it");
        check(Unbound::nativeKeyWindowNumber()==key && custom->window()==&main,"native root transition retains the same native window and focus");
        check(lease.restore(),"native page layout restores"); QApplication::processEvents();
        check(page->layout()==original && nativeControl->parentWidget()==page && nativeControl->isVisible() &&
            unmanaged->isHidden()==unmanagedHidden && page->minimumSize()==minimum && page->sizePolicy()==policy,
            "native root recovery returns the original layout object, controls, visibility and constraints");
        check(arranged->parentWidget()==custom && custom->parentWidget()==&main && !custom->isWindow(),
            "restoring the native root retains the custom tree for its caller to release safely");
    }
    {
        QWidget main; auto page=new QWidget(&main); auto original=new QVBoxLayout(page);
        QPointer<QWidget> native=new QPushButton("Native owner lifetime",page); original->addWidget(native);
        auto custom=new QWidget(&main); auto layout=new QVBoxLayout(custom);
        QPointer<QWidget> arranged=new QPushButton("Arranged lifetime",custom); layout->addWidget(arranged);
        Unbound::NativeRootLayout lease; check(lease.mount(page,custom).isEmpty(),"native root destruction setup");
        delete page; QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        check(!native && !arranged && !lease.active() && !main.findChild<QWidget *>("unboundNativeLayoutStash"),
            "closing a native page destroys both its current and parked content without orphaned controls");
    }
    {
        QWidget main; QWidget page(&main); auto original=new QVBoxLayout(&page);
        QPointer<QWidget> native=new QPushButton("Native retained",&page); original->addWidget(native);
        auto custom=new QWidget(&main); auto customLayout=new QVBoxLayout(custom);
        QPointer<QWidget> arranged=new QPushButton("Arranged retained",custom); customLayout->addWidget(arranged);
        Unbound::NativeRootLayout lease; check(lease.mount(&page,custom).isEmpty(),"foreign layout replacement setup");
        QWidget parking(&main); parking.setLayout(page.layout()); auto foreign=new QHBoxLayout(&page);
        check(!lease.restore() && page.layout()==foreign && native && arranged,
            "unexpected native layout replacement rejects restoration without deleting either control tree");
        delete foreign; page.setLayout(parking.layout());
        check(lease.restore() && page.layout()==original,"restoration can retry when expected layout ownership returns");
    }
    {
        auto main=new QWidget; auto slot=new Unbound::NativePanelSlot(main); auto surface=new QWidget(main);
        QPointer<QWidget> content=new QPushButton("Native lifetime",surface);
        Unbound::NativeGroupSlot mount; check(mount.mount(slot,surface,main).isEmpty(),"native lifetime mount setup");
        delete main; mount.release();
        check(!content && !mount.active(),"native owner destruction cannot orphan or resurrect its panel content");
    }
    {
        QWidget main; QWidget page(&main); auto original=new QVBoxLayout(&page);
        auto custom=new QWidget(&main); auto customLayout=new QVBoxLayout(custom);
        QHash<QString,QWidget *> leaves;
        for (const auto &id : {QStringLiteral("source"),QStringLiteral("inspector"),QStringLiteral("program")})
            leaves[id]=new QPushButton(id,custom);
        Unbound::PanelLayoutCallbacks callbacks;
        callbacks.leaf=[&](const QString &id) { return leaves.value(id); };
        callbacks.title=[](const QString &id) { return id; };
        const QJsonObject tree{{"orientation","vertical"},{"children",QJsonArray{
            QJsonObject{{"orientation","tabs"},{"selected",1},{"children",QJsonArray{
                QJsonObject{{"panel","source"}},QJsonObject{{"panel","inspector"}}}}},
            QJsonObject{{"panel","program"}}}}};
        auto rendered=Unbound::buildPanelLayoutSurface(tree,custom,custom,callbacks);
        check(rendered,"shared split/tab renderer accepts a native arrangement tree");
        if (rendered) {
            customLayout->addWidget(rendered); Unbound::NativeRootLayout lease;
            check(lease.mount(&page,custom).isEmpty(),"rendered nested arrangement mounts inside native page");
            auto tabs=custom->findChild<QTabWidget *>();
            check(tabs && tabs->currentWidget()==leaves.value("inspector") &&
                leaves.value("program")->window()==&main,"native arrangement preserves selected tab and common native window");
            check(lease.restore() && page.layout()==original &&
                leaves.value("source")->parentWidget() && custom->isAncestorOf(leaves.value("program")),
                "native recovery preserves all nested real controls for session restoration");
        }
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto nativeLayout=new QVBoxLayout(page); auto panel=new QPushButton("Native session control",page);
        nativeLayout->addWidget(panel); main.show(); QApplication::processEvents();
        Unbound::MacPanelSession session;
        check(session.undock(panel,page,"Native session").isEmpty(),"native arrangement session starts with a reversible native slot");
        auto custom=new QWidget(&main); auto row=new QVBoxLayout(custom); auto cell=new QWidget(custom); row->addWidget(cell);
        QWidget unrelated(&main);
        check(!session.attachToGroup(&unrelated).isEmpty(),"unregistered native children cannot adopt panel sessions");
        Unbound::NativeRootLayout lease; check(lease.mount(page,custom).isEmpty(),"native session root mount");
        check(session.attachToGroup(cell).isEmpty(),"registered native arrangement adopts actual session content");
        session.finishGroupTransfer(); QApplication::processEvents();
        check(session.placement()==Unbound::PanelPlacement::NativeSurface && panel->window()==&main && panel->isVisible(),
            "embedded session reports native placement and retains its working control in the original window");
        const auto frame=main.geometry();
        session.setHiddenByUser(true); session.setHiddenByUser(false); QApplication::processEvents();
        check(session.placement()==Unbound::PanelPlacement::NativeSurface && main.geometry()==frame && panel->isVisible(),
            "native session door visibility retains its placement and outer window geometry");
        check(session.beginGroupedExpansion(),"native embedded session can temporarily expand without relinquishing its native slot");
        session.endGroupedExpansion(); QApplication::processEvents();
        check(session.window()==cell && panel->window()==&main && cell->layout(),
            "temporary expansion returns the actual control layout to its same embedded cell");
        session.detachFromGroup();
        check(session.floating() && panel->window()!=&main && lease.active(),
            "detaching one session leaves the native arrangement owner intact");
        check(lease.restore() && !Unbound::NativeRootLayout::contains(page,cell),"native restoration revokes the arrangement adoption scope");
        check(session.dock().isEmpty() && panel->parentWidget()==page && page->layout()==nativeLayout,
            "panel session returns to the exact restored native layout after arrangement recovery");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto original=new QHBoxLayout(page); auto remaining=new QPushButton("Remaining native panel",page); original->addWidget(remaining);
        auto unmanaged=new QPushButton("Native overlay",page); unmanaged->setGeometry(3,4,120,28);
        auto hidden=new QPushButton("Hidden native control",page); hidden->hide();
        const auto overlayGeometry=unmanaged->geometry();
        auto custom=new QWidget(&main); auto row=new QHBoxLayout(custom); auto remainder=new QWidget(custom); row->addWidget(remainder);
        auto arranged=new QPushButton("Arranged panel",custom); row->addWidget(arranged);
        main.resize(800,450); main.show(); QApplication::processEvents();
        Unbound::NativeRootLayout lease; check(lease.mount(page,custom).isEmpty(),"remaining native content mount setup");
        QWidget unrelated(&main);
        check(!lease.retainNativeRemainder(&unrelated).isEmpty(),"native remainder cannot be moved outside its registered arrangement");
        check(lease.retainNativeRemainder(remainder).isEmpty(),"remaining native controls occupy a real arrangement leaf");
        QApplication::processEvents();
        check(remaining->isVisible() && arranged->isVisible() && unmanaged->isVisible() && hidden->isHidden() &&
            remaining->window()==&main && remainder->isAncestorOf(remaining),
            "partial rearrangement preserves remaining panels and unmanaged controls without another window");
        int calls=0; QObject::connect(remaining,&QPushButton::clicked,&main,[&] { ++calls; }); remaining->click();
        check(calls==1,"remaining native controls retain their original signal connections");
        check(!lease.retainNativeRemainder(new QWidget(custom)).isEmpty(),"remaining native tree has only one arrangement owner");
        check(lease.restore() && page->layout()==original && remaining->parentWidget()==page && unmanaged->parentWidget()==page &&
            unmanaged->geometry()==overlayGeometry && hidden->parentWidget()==page && hidden->isHidden(),
            "native recovery restores remainder controls, unmanaged geometry and hidden states exactly");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto original=new QHBoxLayout(page); auto panel=new QPushButton("Returning native panel",page);
        auto remaining=new QPushButton("Remaining native panel",page); original->addWidget(panel); original->addWidget(remaining);
        main.show(); QApplication::processEvents(); Unbound::MacPanelSession session;
        check(session.undock(panel,page,"Returning panel").isEmpty(),"individual native return starts with retained slot");
        auto custom=new QWidget(&main); auto row=new QVBoxLayout(custom); auto remainder=new QWidget(custom); row->addWidget(remainder);
        Unbound::NativeRootLayout lease;
        check(lease.mount(page,custom).isEmpty() && lease.retainNativeRemainder(remainder).isEmpty(),"individual return keeps the remaining native layout visible");
        check(session.dock().isEmpty() && !session.active() && remainder->isAncestorOf(panel) && lease.active(),
            "a floating panel returns to its original slot while the native arrangement remains active");
        QApplication::processEvents();
        check(panel->isVisible() && remaining->isVisible() && panel->window()==&main,
            "returning a panel preserves remaining native controls in the same window");
        check(lease.restore() && page->layout()==original && panel->parentWidget()==page && remaining->parentWidget()==page,
            "global recovery restores both individually returned and remaining native panels");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto original=new QHBoxLayout(page); auto source=new QPushButton("Source",page); auto inspector=new QPushButton("Inspector",page);
        auto remaining=new QPushButton("Timeline",page); original->addWidget(source); original->addWidget(inspector); original->addWidget(remaining);
        auto media=new QPushButton("Media Pool",page); auto effects=new QPushButton("Effects",page); original->addWidget(media); original->addWidget(effects);
        main.resize(900,500); main.show(); QApplication::processEvents();
        Unbound::MacPanelSession s,i; check(s.undock(source,page,"Source").isEmpty() && i.undock(inspector,page,"Inspector").isEmpty(),"native coordinator panel setup");
        Unbound::NativeArrangement arrangement;
        const auto tree=Unbound::splitGroupTree({{"panel",Unbound::NativeArrangement::nativeLeaf()}},
            Unbound::splitGroupTree({{"panel","source"}},{{"panel","inspector"}},Qt::Orientation(0)),Qt::Horizontal);
        check(arrangement.initialize(page,{{"source",&s},{"inspector",&i}},tree).isEmpty(),"page coordinator adopts multiple sessions and preserves the native remainder");
        QApplication::processEvents();
        if (!source->isVisible() || inspector->isVisible() || !remaining->isVisible())
            std::fprintf(stderr,"Native coordinator visibility source=%d inspector=%d remainder=%d placement=%d/%d\n",
                source->isVisible(),inspector->isVisible(),remaining->isVisible(),int(s.placement()),int(i.placement()));
        check(source->isVisible() && !inspector->isVisible() && remaining->isVisible() &&
            s.placement()==Unbound::PanelPlacement::NativeSurface && i.placement()==Unbound::PanelPlacement::NativeSurface,
            "native coordinator presents only the selected tab beside the remaining native controls");
        const auto frame=main.geometry();
        bool rejectedReentry=false; i.changed=[&] { rejectedReentry=!arrangement.restore(true); };
        check(arrangement.setPanelVisible("inspector",true) && inspector->isVisible() && !source->isVisible(),
            "native arrangement reveal selects the Inspector tab");
        i.changed={};
        check(rejectedReentry && arrangement.active(),"native command notifications cannot reenter and destroy an in-progress arrangement transaction");
        check(arrangement.setPanelVisible("inspector",false) && source->isVisible() && !inspector->isVisible(),
            "native arrangement door returns to the previously selected Source tab");
        check(arrangement.setPanelVisible("inspector",true) && main.geometry()==frame,
            "native arrangement door cycling does not resize the outer native window");
        Unbound::activateNativeWindowForTest(&main);
        QElapsedTimer presentation; presentation.start();
        const auto number=Unbound::nativeWindowNumber(&main);
        while (presentation.elapsed()<1000) {
            QApplication::processEvents(); const auto order=Unbound::nativeOrderedWindowNumbers();
            if (!order.isEmpty() && order.first()==number) break;
            QThread::msleep(1);
        }
        const auto targets=Unbound::nativeDockSurfaces(&main);
        check(!targets.isEmpty(),"native arrangement registers visible leaf destinations");
        if (!targets.isEmpty()) {
            auto target=targets.first(); const QRect targetFrame(target->mapToGlobal(QPoint()),target->size());
            const auto hit=Unbound::resolveDockDestination(nullptr,targetFrame.center(),{});
            check(hit.surface==target && hit.frame==targetFrame,"native drop lookup resolves a real leaf with its exact client bounds");
        }
        check(!Unbound::resolveDockDestination(nullptr,main.mapToGlobal(QPoint(1,1)),{}).surface,
            "unused native window edges never become implicit drop zones");
        check(!Unbound::registerNativeDockSurfaces(&main,&main,{arrangement.surface()}),
            "the whole native main window cannot register itself as a docking surface");
        Unbound::MacPanelSession m,e;
        check(m.undock(media,page,"Media Pool").isEmpty() && e.undock(effects,page,"Effects").isEmpty(),
            "panels can detach from the retained native remainder after arrangement creation");
        const auto addition=Unbound::splitGroupTree({{"panel","media"}},{{"panel","effects"}},Qt::Vertical);
        const auto beforeRejected=arrangement.snapshot();
        check(!arrangement.add({{"media",&m},{"effects",&e}},addition,"missing-target",Qt::Horizontal).isEmpty() &&
            arrangement.snapshot()==beforeRejected && m.floating() && e.floating(),
            "invalid dock targets reject a multi-panel transfer before changing either arrangement");
        check(arrangement.add({{"media",&m},{"effects",&e}},addition,"inspector",Qt::Horizontal).isEmpty(),
            "a multi-panel split subtree can join an existing nested native arrangement");
        QStringList mergedNames;
        check(Unbound::groupTreeNames(arrangement.snapshot(),mergedNames) && mergedNames.size()==5 &&
            m.placement()==Unbound::PanelPlacement::NativeSurface && e.placement()==Unbound::PanelPlacement::NativeSurface,
            "merging preserves the native remainder and all four independent panel leases");
        check(arrangement.setPanelVisible("effects",false),"native split panel can close before saving a preset");
        const auto saved=arrangement.savedPlan("edit"); Unbound::NativeArrangementPlan savedPlan;
        check(Unbound::NativeArrangementPlan::parse(saved,"edit",{"source","inspector","media","effects"},savedPlan).isEmpty() &&
            savedPlan.hidden.contains("effects"),"closed native split panels retain valid reopen proportions in the saved plan");
        check(arrangement.detach("source") && s.floating() && arrangement.active(),"one panel can float while the other panel and native remainder remain arranged");
        check(s.dock().isEmpty() && remaining->isVisible() && arrangement.active(),"a detached panel can return to its retained native slot without destroying the arrangement");
        check(arrangement.restore(true) && !i.active() && !m.active() && !e.active() && page->layout()==original &&
            source->parentWidget()==page && inspector->parentWidget()==page && remaining->parentWidget()==page &&
            media->parentWidget()==page && effects->parentWidget()==page,
            "coordinator restores native root before docking every remaining owned session");
        check(Unbound::nativeDockSurfaces(&main).isEmpty(),"native recovery removes all custom drop destinations");
        const auto prepare=[&](Unbound::MacPanelSession &session,QWidget *panel,const QString &title) {
            const auto error=session.undock(panel,page,title);
            if (!error.isEmpty()) std::fprintf(stderr,"Saved plan prepare %s: %s visible=%d hidden=%d page=%d active=%d\n",
                qPrintable(title),qPrintable(error),panel->isVisible(),panel->isHidden(),page->isVisible(),session.active());
            return error.isEmpty();
        };
        const bool preparedSource=prepare(s,source,"Source"),preparedInspector=prepare(i,inspector,"Inspector"),
            preparedMedia=prepare(m,media,"Media Pool"),preparedEffects=prepare(e,effects,"Effects");
        check(preparedSource && preparedInspector && preparedMedia && preparedEffects,"saved native plan session preparation");
        check(arrangement.initializeSaved(page,{{"source",&s},{"inspector",&i},{"media",&m},{"effects",&e}},saved,"edit").isEmpty() &&
            e.hiddenByUser() && !effects->isVisible() && i.placement()==Unbound::PanelPlacement::NativeSurface,
            "saved native plans restore nested placement and closed panels without selecting every revealed tab");
        check(arrangement.restore(true) && page->layout()==original && effects->parentWidget()==page,
            "saved-plan restoration remains reversible to the same native slots");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto original=new QVBoxLayout(page); QPointer<QWidget> panel=new QPushButton("Project lifetime",page); original->addWidget(panel);
        main.show(); QApplication::processEvents(); Unbound::MacPanelSession session; Unbound::NativeArrangement arrangement;
        check(session.undock(panel,page,"Lifetime").isEmpty(),"coordinator lifetime session setup");
        const auto tree=Unbound::splitGroupTree({{"panel","panel"}},{{"panel",Unbound::NativeArrangement::nativeLeaf()}},Qt::Horizontal);
        check(arrangement.initialize(page,{{"panel",&session}},tree).isEmpty(),"coordinator lifetime mount");
        delete page; QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        check(!panel && !arrangement.active() && !session.active() && !session.releaseGroup,
            "project page destruction clears native coordinator leases and callbacks without reviving content");
        auto nextPage=new QWidget(&main); outer->addWidget(nextPage); auto nextLayout=new QVBoxLayout(nextPage);
        auto nextPanel=new QPushButton("Next project",nextPage); nextLayout->addWidget(nextPanel); nextPage->show(); QApplication::processEvents();
        check(session.undock(nextPanel,nextPage,"Next project").isEmpty() && arrangement.initialize(nextPage,{{"panel",&session}},tree).isEmpty(),
            "the same coordinator can adopt a new project after the old page is destroyed");
        check(arrangement.restore(true) && nextPanel->parentWidget()==nextPage,"replacement project's native layout restores independently");
    }
    {
        QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);auto outer=new QVBoxLayout(&main);
        auto page=new QWidget(&main);auto native=new QHBoxLayout(page);outer->addWidget(page);
        auto program=new QPushButton("Program",page);auto timeline=new QPushButton("Timeline",page);auto source=new QPushButton("Source",page);
        native->addWidget(program);native->addWidget(timeline);native->addWidget(source);main.resize(1000,600);main.show();QApplication::processEvents();
        Unbound::MacPanelSession p,t,s;Unbound::NativeArrangement arrangement(nullptr);
        check(arrangement.preparePosition(program,page,&p,"program","Program").isEmpty(),"scoped Program native position prepared");
        check(t.undock(timeline,page,"Timeline").isEmpty(),"Timeline detaches for scoped arrangement");
        const auto tree=Unbound::splitGroupTree({{"panel","program"}},{{"panel","timeline"}},Qt::Vertical);
        check(arrangement.initialize(page,{{"program",&p},{"timeline",&t}},tree).isEmpty(),"scoped arrangement mounts under native marker");
        check(arrangement.surface()->parentWidget()!=page,"fixture distinguishes native slot parent from real page identity");
        check(s.undock(source,page,"Source").isEmpty(),"Source detaches from the same original page");
        check(arrangement.add({{"source",&s}},{{"panel","source"}},"program",Qt::Horizontal).isEmpty(),"same-page Source joins scoped Program arrangement");
        check(arrangement.contains("source")&&s.placement()==Unbound::PanelPlacement::NativeSurface,"Source is adopted by the intended native arrangement");
        QWidget foreignPage(&main);auto foreignLayout=new QVBoxLayout(&foreignPage);auto foreign=new QPushButton("Foreign",&foreignPage);foreignLayout->addWidget(foreign);foreignPage.show();QApplication::processEvents();
        Unbound::MacPanelSession f;check(f.undock(foreign,&foreignPage,"Foreign").isEmpty(),"foreign-page negative fixture prepared");
        const auto before=arrangement.snapshot();
        check(!arrangement.add({{"foreign",&f}},{{"panel","foreign"}},"program",Qt::Horizontal).isEmpty()&&arrangement.snapshot()==before&&f.floating(),"foreign-page transfer remains rejected without mutation");
        check(f.dock().isEmpty(),"foreign test session returns safely");
        check(arrangement.restore(true)&&!p.active()&&!t.active()&&!s.active()&&program->parentWidget()==page&&source->parentWidget()==page,"scoped arrangement including newly added Source restores original ownership");
    }
    Unbound::flushDiagnostics(); QFile events(files.path()+"/events.jsonl");
    check(events.open(QIODevice::ReadOnly),"native arrangement diagnostic log is readable");
    const auto trace=events.readAll();
    check(trace.contains("workspace.native-root.remainder") && trace.contains("workspace.native-arrangement.create") &&
        trace.contains("workspace.native-arrangement.visibility") && trace.contains("workspace.native-arrangement.detach") &&
        trace.contains("workspace.native-arrangement.owner-destroyed") && trace.contains("work_ms") && trace.contains("outer_frame_changed"),
        "native arrangement transitions retain ownership, visibility, lifetime and work-duration diagnostics");
    if (!failures) std::puts("PASS: embedded native layout ownership, constraints, visibility and destruction");
    return failures ? 1 : 0;
}
