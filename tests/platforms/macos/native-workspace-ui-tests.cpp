#include "native-fixture-start.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include <execinfo.h>
#include "panelcontrols.h"
static QtMessageHandler previousMessages=nullptr;
static int nativeRecreationWarnings=0;
static void traceNativeWarning(QtMsgType type,const QMessageLogContext &context,const QString &message) {
    if (message.contains("active key-value observers")) {
        ++nativeRecreationWarnings;
        if (qEnvironmentVariableIsSet("UNBOUND_TRACE_NATIVE_WARNINGS")) {
            void *frames[28]; const int count=backtrace(frames,28); backtrace_symbols_fd(frames,count,2);
        }
    }
    if (previousMessages) previousMessages(type,context,message);
    else std::fprintf(stderr,"%s\n",qPrintable(message));
}
class StandaloneShows : public QObject {
public:
    int count=0;
    bool eventFilter(QObject *object,QEvent *event) override {
        if (event->type()==QEvent::Show && (object->objectName()=="unboundFloating_frameEditPageMediaPool" ||
            object->objectName()=="unboundFloating_widgetEditInspector")) ++count;
        return false;
    }
};
static void settle(int duration=50) {
    QElapsedTimer timer; timer.start();
    do { QApplication::processEvents(); QThread::msleep(1); } while (timer.elapsed()<duration);
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false); QTemporaryDir files;
    previousMessages=qInstallMessageHandler(traceNativeWarning);
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());
    qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());
    int failures=0; auto check=[&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    QWidget main; main.setObjectName("UiMainWindow"); auto outer=new QVBoxLayout(&main);
    auto page=new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page); auto native=new QHBoxLayout(page);
    auto media=new QWidget(page); media->setObjectName("frameEditPageMediaPool"); auto contents=new QVBoxLayout(media);
    auto header=new QWidget(media); auto row=new QHBoxLayout(header); auto options=new QToolButton(header); options->setObjectName("btnMediaPoolOptions");
    auto menu=new QMenu(options); options->setMenu(menu); row->addWidget(options); contents->addWidget(header);
    QPointer<QPushButton> mediaControl=new QPushButton("Native media control",media); contents->addWidget(mediaControl); native->addWidget(media);
    auto remaining=new QPushButton("Native remaining panel",page); native->addWidget(remaining);
    auto inspector=new QWidget(page); inspector->setObjectName("widgetEditInspector");
    auto inspectorLayout=new QVBoxLayout(inspector); auto inspectorHeader=new QWidget(inspector); auto inspectorRow=new QHBoxLayout(inspectorHeader);
    auto inspectorOptions=new QToolButton(inspectorHeader); inspectorOptions->setObjectName("btnTitleOptions"); inspectorRow->addWidget(inspectorOptions); inspectorLayout->addWidget(inspectorHeader);
    QPointer<QPushButton> inspectorControl=new QPushButton("Native Inspector control",inspector); inspectorLayout->addWidget(inspectorControl); native->addWidget(inspector);
    auto callers=new QMenu(&main); callers->setObjectName("workspaceShowPanel");
    main.resize(1100,650); main.show(); settle();
    {
        prepareNativeFixture();PanelControls controls; settle(300);
        auto undock=controls.findChild<QPushButton *>("undock_widgetEditInspector"); if (undock) undock->click(); settle();
        auto floating=dynamic_cast<Unbound::MacFloatingHost *>(inspectorControl->window());
        check(floating && floating->attachRequested && Unbound::nativeDockSurfaces(&main).contains(media),
            "visible supported native panels are drop targets before any custom arrangement exists");
        controls.move(1400,800); if (floating) floating->move(1300,400); main.raise(); settle();
        QElapsedTimer destinationReady; destinationReady.start(); Unbound::DockDestination destination;
        do { destination=Unbound::resolveDockDestination(floating,media->mapToGlobal(media->rect().center()),{});
            if (destination.surface!=media) settle(10);
        } while (destination.surface!=media && destinationReady.elapsed()<500);
        check(destination.surface==media,"native WindowServer destination lookup finds the initial panel target");
        check(!Unbound::resolveDockDestination(floating,main.mapToGlobal(QPoint(2,2)),{}).surface,
            "unused native window margins never become first-drop destinations");
        if (floating && floating->attachRequested) floating->attachRequested(media,Qt::RightEdge); settle();
        auto marker=page->findChild<QWidget *>("unboundNativePanelPosition_frameEditPageMediaPool");
        check(marker && native->indexOf(marker)==0 && native->indexOf(remaining)==1 && remaining->parentWidget()==page &&
            mediaControl->window()==&main && inspectorControl->window()==&main && page->layout()==native,
            "first drop occupies the chosen native panel slot without replacing the page layout or other native panels");
        const auto frame=main.geometry();
        auto hide=controls.findChild<QPushButton *>("hide_widgetEditInspector"),show=controls.findChild<QPushButton *>("show_widgetEditInspector");
        if (hide) hide->click(); settle(); if (show) show->click(); settle();
        check(inspectorControl->isVisible() && main.geometry()==frame,"scoped native doors preserve the outer window frame");
    }
    settle();
    check(media->parentWidget()==page && page->layout()==native && inspector->parentWidget()==page,
        "scoped native shutdown returns both panels to their original native slots");
    {
        prepareNativeFixture();PanelControls controls; settle(400);
        check(page->findChild<QWidget *>("unboundNativePanelPosition_frameEditPageMediaPool") &&
            mediaControl->window()==&main && inspectorControl->window()==&main && page->layout()==native,
            "ordinary helper reopening restores a scoped native first-drop arrangement");
        auto source=controls.findChild<QPushButton *>("undock_widgetEditInspector"); if (source) source->click(); settle();
        auto anchor=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool"); if (anchor) anchor->click(); settle();
        check(mediaControl->window()!=&main && inspectorControl->window()!=&main && remaining->isVisible(),
            "source and final anchor can detach independently while preserving remaining native panels");
        auto dock=controls.findChild<QPushButton *>("bulk_1"); if (dock) dock->click(); settle(300);
        check(media->parentWidget()==page && inspector->parentWidget()==page && native->indexOf(media)==0,
            "Dock All restores the adopted anchor slot after the final scoped panel detaches");
        if (source) source->click(); settle();
        auto floating=dynamic_cast<Unbound::MacFloatingHost *>(inspectorControl->window());
        if (floating && floating->attachRequested) floating->attachRequested(media,Qt::Edge(0)); settle();
        check(page->findChild<QTabWidget *>("unboundPanelTabs") && inspectorControl->isVisible() && !mediaControl->isVisible(),
            "the first native drop can create a selected tab at the target panel's original position");
        auto expand=controls.findChild<QPushButton *>("expand_frameEditPageMediaPool");
        if (expand) expand->click(); settle(80); if (expand) expand->click(); settle();
        check(!mediaControl->isVisible() && inspectorControl->isVisible(),"scoped maximize restores the previously selected native tab");
    }
    settle();
    {
        prepareNativeFixture();PanelControls controls; settle(400);
        check(page->findChild<QWidget *>("unboundNativePanelPosition_frameEditPageMediaPool") &&
            page->findChild<QTabWidget *>("unboundPanelTabs") && inspectorControl->isVisible() && !mediaControl->isVisible(),
            "saved first-drop tabs restore their native anchor and selected panel");
        auto undock=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool"); if (undock) undock->click(); settle();
        check(mediaControl->window()!=&main && mediaControl->isVisible() && inspectorControl->window()==&main,
            "the anchor can float while another member retains the scoped native position");
    }
    settle();
    {
        prepareNativeFixture();PanelControls controls; settle(400);
        if (!mediaControl || !inspectorControl) {
            files.setAutoRemove(false); std::fprintf(stderr,"FAIL: saved scoped restoration deleted a native control; diagnostics: %s\n",qPrintable(files.path())); return 1;
        }
        check(mediaControl->window()!=&main && mediaControl->isVisible() && inspectorControl->window()==&main &&
            page->findChild<QWidget *>("unboundNativePanelPosition_frameEditPageMediaPool"),
            "saved native placement survives with its original anchor floating independently");
        auto dockAnchor=controls.findChild<QPushButton *>("dock_frameEditPageMediaPool"); if (dockAnchor) dockAnchor->click(); settle();
        check(media->parentWidget()==page && inspectorControl->window()!=&main && inspectorControl->isVisible(),
            "returning the anchor to its original native slot releases the remaining group without deleting its content");
        auto dock=controls.findChild<QPushButton *>("bulk_1"); if (dock) dock->click(); settle(300);
    }
    settle();
    {
        prepareNativeFixture();PanelControls controls; settle(300);
        auto undock=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool");
        check(undock && undock->isEnabled(),"native Media Pool has a callable undock command");
        if (undock) undock->click(); settle();
        check(mediaControl->window()!=&main,"native panel command first floats the actual Media Pool content");
        QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection); QAction *dockNative=nullptr;
        for (auto action : menu->actions()) if (action->objectName()=="unboundDockNativeWorkspace") dockNative=action;
        check(dockNative && dockNative->isVisible(),"native options menu exposes Dock in Native Workspace for a floating panel");
        if (dockNative) dockNative->trigger(); settle();
        check(mediaControl->window()==&main && remaining->isVisible() && page->findChild<QWidget *>("unboundNativeArrangement"),
            "native menu command embeds the real panel while preserving remaining native controls");
        const auto frame=main.geometry();
        auto hide=controls.findChild<QPushButton *>("hide_frameEditPageMediaPool");
        auto show=controls.findChild<QPushButton *>("show_frameEditPageMediaPool");
        if (hide) hide->click(); settle(); check(!mediaControl->isVisible() && remaining->isVisible(),"panel manager Hide routes into native arrangement visibility");
        if (show) show->click(); settle();
        if (main.geometry()!=frame) std::fprintf(stderr,"Native UI frame before=%d,%d,%d,%d after=%d,%d,%d,%d\n",frame.x(),frame.y(),frame.width(),frame.height(),
            main.x(),main.y(),main.width(),main.height());
        check(mediaControl->isVisible() && main.geometry()==frame,"panel manager Show recalls native placement without changing the outer frame");
        auto undockInspector=controls.findChild<QPushButton *>("undock_widgetEditInspector");
        if (undockInspector) undockInspector->click(); settle();
        auto target=page->findChild<QWidget *>("unboundGroupCell_frameEditPageMediaPool");
        auto source=dynamic_cast<Unbound::MacFloatingHost *>(inspectorControl->window());
        check(source && source->attachRequested && target,"native arrangement exposes a real cell to floating-panel drop dispatch");
        if (source && source->attachRequested && target) source->attachRequested(target,Qt::Edge(0)); settle();
        check(inspectorControl->window()==&main && inspectorControl->isVisible() && !mediaControl->isVisible(),
            "floating Inspector drop creates a selected native tab beside Media Pool");
        QAction *inspectorCaller=nullptr;
        for (auto action : callers->actions()) if (action->objectName()=="unboundCaller_widgetEditInspector") inspectorCaller=action;
        check(inspectorCaller,"Workspace panel submenu retains an Inspector caller in a native tab group");
        if (inspectorCaller) inspectorCaller->trigger(); settle();
        check(!inspectorControl->isVisible() && mediaControl->isVisible(),"Workspace Inspector command closes its door and returns to Media Pool");
        if (inspectorCaller) inspectorCaller->trigger(); settle();
        check(inspectorControl->isVisible() && !mediaControl->isVisible(),"Workspace Inspector command reveals and selects its retained native tab");
        auto expandMedia=controls.findChild<QPushButton *>("expand_frameEditPageMediaPool");
        if (expandMedia) expandMedia->click(); settle(80);
        check(mediaControl->window()!=&main && inspectorControl->window()==&main,"temporary expansion borrows only the requested native tab's content");
        if (expandMedia) expandMedia->click(); settle();
        check(mediaControl->window()==&main && !mediaControl->isVisible() && inspectorControl->isVisible() && main.geometry()==frame,
            "native maximize restoration returns exact tab selection and outer frame");
        auto hideAll=controls.findChild<QPushButton *>("bulk_3"); if (hideAll) hideAll->click(); settle();
        auto root=page->findChild<QWidget *>("unboundNativeArrangement"); auto split=root ? root->findChild<QSplitter *>() : nullptr;
        check(!mediaControl->isVisible() && !inspectorControl->isVisible() && remaining->isVisible() && split && split->sizes().size()==2 && split->sizes()[1]==0,
            "Hide All collapses the empty native tab branch while preserving remaining native controls");
        if (inspectorCaller) inspectorCaller->trigger(); settle();
        check(inspectorControl->isVisible() && main.geometry()==frame,"Workspace caller reopens an entirely closed native branch without resizing the outer frame");
    }
    settle(); check(media->parentWidget()==page && page->layout()==native,"helper shutdown returns the exact native layout and panel slot");
    QFile saved(files.path()+"/Workspace.json"); saved.open(QIODevice::ReadOnly);
    const auto workspace=QJsonDocument::fromJson(saved.readAll()).object();
    check(workspace["native_arrangement"].toObject()["panels"].toObject().contains("frameEditPageMediaPool"),
        "normal workspace saving retains the native arrangement plan");
    {
        prepareNativeFixture();PanelControls controls; settle(400);
        check(mediaControl->window()==&main && page->findChild<QWidget *>("unboundNativeArrangement") && remaining->isVisible(),
            "helper reopening restores saved native placement through ordinary startup commands");
        auto undock=controls.findChild<QPushButton *>("undock_frameEditPageMediaPool");
        check(undock && undock->isEnabled(),"embedded panel retains a discoverable Undock command");
        if (undock) undock->click(); settle();
        check(mediaControl->window()!=&main && mediaControl->isVisible() && inspectorControl->window()==&main,
            "native arrangement Undock reveals its floating content while leaving other native tabs in place");
        auto inspectorCell=main.findChild<QWidget *>("unboundGroupCell_widgetEditInspector");
        auto glyph=inspectorCell?inspectorCell->findChild<QToolButton *>("unboundDetachPanel"):nullptr;
        check(glyph && glyph->isVisible(),"embedded Inspector exposes the shared single-header Undock glyph");
        if (glyph) glyph->click(); settle();
        check(inspectorControl->window()!=&main && inspectorControl->isVisible() && !page->findChild<QWidget *>("unboundNativeArrangement"),
            "native header Undock floats the final tab and releases the empty custom arrangement");
        auto first=controls.findChild<QComboBox *>("groupFirst");
        auto second=controls.findChild<QComboBox *>("groupSecond");
        if (first) first->setCurrentIndex(first->findText("Media Pool"));
        if (second) second->setCurrentIndex(second->findText("Inspector"));
        auto tabs=controls.findChild<QPushButton *>("group_3");
        if (tabs) tabs->click(); settle(200);
        check(mediaControl->window()==inspectorControl->window() && mediaControl->window()!=&main,
            "two floating panels form a tabbed group before native transfer");
        StandaloneShows standalone; app.installEventFilter(&standalone);
        QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection);
        for (auto action : menu->actions()) if (action->objectName()=="unboundDockNativeWorkspace") action->trigger();
        settle();
        app.removeEventFilter(&standalone);
        check(mediaControl->window()==&main && inspectorControl->window()==&main && page->findChild<QWidget *>("unboundNativeArrangement"),
            "a native menu action retained across helper reopening reconnects to the new helper");
        check(standalone.count==0,"floating group transfer never shows intermediate standalone member windows");
        auto nativeTabs=page->findChild<QTabWidget *>("unboundPanelTabs");
        check(nativeTabs && nativeTabs->count()==2,"native group retains draggable panel tabs");
        if (nativeTabs) {
            auto bar=nativeTabs->tabBar(); const auto start=bar->tabRect(0).center();
            const auto end=QPoint(start.x(),bar->height()+40);
            QMouseEvent press(QEvent::MouseButtonPress,start,bar->mapToGlobal(start),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
            QMouseEvent move(QEvent::MouseMove,end,bar->mapToGlobal(end),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(bar,&press); QApplication::sendEvent(bar,&move); settle();
            const bool mediaFloating=mediaControl->window()!=&main;
            const bool inspectorFloating=inspectorControl->window()!=&main;
            check(mediaFloating!=inspectorFloating,"dragging a native tab outside its strip floats only that panel");
            if (auto host=dynamic_cast<Unbound::MacFloatingHost *>((mediaFloating ? mediaControl : inspectorControl)->window())) {
                QMouseEvent release(QEvent::MouseButtonRelease,QPoint(80,12),host->mapToGlobal(QPoint(80,12)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
                QApplication::sendEvent(host,&release);
            }
            settle();
        }
    }
    {
        QWidget nativeMain; auto outer=new QVBoxLayout(&nativeMain); auto nativePage=new QWidget(&nativeMain); outer->addWidget(nativePage);
        auto original=new QHBoxLayout(nativePage);
        QList<QWidget *> panels;
        for (const auto &id : {"source","inspector","program"}) {
            auto panel=new QPushButton(id,nativePage); panel->setObjectName(id); original->addWidget(panel); panels.append(panel);
        }
        nativeMain.resize(1000,600); nativeMain.show(); settle();
        Unbound::MacPanelSession source,inspector,program;
        check(source.undock(panels[0],nativePage,"Source").isEmpty() && inspector.undock(panels[1],nativePage,"Inspector").isEmpty() &&
            program.undock(panels[2],nativePage,"Program").isEmpty(),"nested native tear-off session preparation");
        source.window()->setObjectName("unboundFloating_source"); inspector.window()->setObjectName("unboundFloating_inspector");
        program.window()->setObjectName("unboundFloating_program");
        Unbound::NativeWorkspaceBinding binding(&nativeMain);
        binding.registerPanel("source",&source); binding.registerPanel("inspector",&inspector); binding.registerPanel("program",&program);
        int nativeMounts=0; binding.beforeMount=[&] { ++nativeMounts; };
        check(!binding.dock(nativePage,{{"source",&source}},{{"panel","source"}},"missing",Qt::Horizontal,false).isEmpty() && nativeMounts==0 &&
            nativePage->layout()==original && source.floating(),"an unavailable first-drop target cannot silently become a whole-page arrangement");
        auto section=Unbound::splitGroupTree({{"panel","source"}},{{"panel","inspector"}},Qt::Vertical);
        section["sizes"]=QJsonArray{3,2};
        const auto tabs=Unbound::splitGroupTree(section,{{"panel","program"}},Qt::Orientation(0));
        check(binding.dock(nativePage,{{"source",&source},{"inspector",&inspector},{"program",&program}},tabs,
            Unbound::nativeRemainderId(),Qt::Horizontal,false).isEmpty(),"nested split section occupies a native tab");
        settle(); QPointer<Unbound::PanelGroup> detached;
        binding.sectionDetached=[&](Unbound::PanelGroup *group) { detached=group; };
        const auto beforeInvalid=binding.saved();
        check(!binding.detachSection(Unbound::splitGroupTree({{"panel","source"}},{{"panel","missing"}},Qt::Vertical),QRect(0,0,500,500),QPoint()).isEmpty() &&
            binding.saved()==beforeInvalid && source.placement()==Unbound::PanelPlacement::NativeSurface,
            "a stale section request is rejected before changing native ownership or layout");
        int inspectorClicks=0;
        QObject::connect(static_cast<QPushButton *>(panels[1]),&QPushButton::clicked,&nativeMain,[&] { ++inspectorClicks; });
        auto nativeTabs=nativePage->findChild<QTabWidget *>("unboundPanelTabs");
        check(nativeTabs,"nested native section exposes a tab strip");
        const auto beforeSection=nativeTabs ? Unbound::panelLayoutSnapshot(nativeTabs->widget(0)) : QJsonObject();
        if (nativeTabs) {
            auto bar=nativeTabs->tabBar(); const auto start=bar->tabRect(0).center(),end=QPoint(start.x(),bar->height()+40);
            QMouseEvent press(QEvent::MouseButtonPress,start,bar->mapToGlobal(start),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
            QMouseEvent move(QEvent::MouseMove,end,bar->mapToGlobal(end),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(bar,&press); QApplication::sendEvent(bar,&move); settle();
        }
        check(detached && source.floating() && inspector.floating() && source.window()->window()==inspector.window()->window() &&
            program.placement()==Unbound::PanelPlacement::NativeSurface,"dragging a nested native tab transfers its whole split section while leaving other tabs native");
        if (detached) {
            const auto tree=Unbound::PanelGroup::savedTree(detached->snapshot());
            check(tree["orientation"]=="vertical" && tree["children"].toArray()[0].toObject()["panel"]=="source" &&
                tree["children"].toArray()[1].toObject()["panel"]=="inspector","nested native tear-off preserves split direction and panel order");
            const auto oldSizes=beforeSection["sizes"].toArray(),newSizes=tree["sizes"].toArray();
            check(oldSizes.size()==2 && newSizes.size()==2 &&
                qAbs(double(oldSizes[0].toInt())/(oldSizes[0].toInt()+oldSizes[1].toInt())-
                     double(newSizes[0].toInt())/(newSizes[0].toInt()+newSizes[1].toInt()))<0.02,
                "nested native tear-off retains the user's divider proportion");
            static_cast<QPushButton *>(panels[1])->click();
            check(inspectorClicks==1,"native control signal connections survive nested section transfer");
            auto host=static_cast<Unbound::MacFloatingHost *>(detached->window());
            QMouseEvent release(QEvent::MouseButtonRelease,QPoint(80,12),host->mapToGlobal(QPoint(80,12)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(host,&release); detached->dissolve(true); delete detached.data();
        }
        check(binding.release(true) && nativePage->layout()==original && panels[0]->parentWidget()==nativePage && panels[1]->parentWidget()==nativePage &&
            panels[2]->parentWidget()==nativePage,"nested tear-off and native recovery preserve all original panel slots");
    }
    Unbound::flushDiagnostics(); QFile events(files.path()+"/events.jsonl"); events.open(QIODevice::ReadOnly); const auto trace=events.readAll();
    check(trace.contains("workspace.native-transfer.request") && trace.contains("workspace.native-command.complete") &&
        trace.contains("workspace.native-plan.restore.request") && trace.contains("workspace.native-arrangement.presentation"),
        "connected native UI commands retain transfer, completion, restore and presentation diagnostics");
    check(nativeRecreationWarnings==0,"Inspector transfers never recreate an observed native window");
    if (!failures) std::puts("PASS: native menu docking, panel doors, workspace saving, helper reopen and native undock commands");
    else { files.setAutoRemove(false); std::fprintf(stderr,"Native UI failure diagnostics retained: %s\n",qPrintable(files.path())); }
    return failures ? 1 : 0;
}
