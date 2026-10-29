#include "native-fixture-start.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "panelgroup.h"
#include "panelcontrols.h"

static int failures = 0;
static void check(bool pass, const char *message)
{ if (!pass) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); } }
static void settle(int duration = 30)
{
    QElapsedTimer timer; timer.start();
    do { QCoreApplication::processEvents(); QThread::msleep(1); } while (timer.elapsed() < duration);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
int main(int argc, char **argv)
{
    QApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir temporary;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", temporary.path().toUtf8());
    qputenv("UNBOUND_WORKSPACE_FILE", (temporary.path() + "/Workspace.json").toUtf8());
    qputenv("UNBOUND_ARRANGEMENTS_FILE", (temporary.path() + "/Arrangements.json").toUtf8());
    {
        QWidget main; QWidget cell(&main); cell.setObjectName("unboundGroupCell_clipFixture");
        check(Unbound::setNativeGroupSurfaceClipping(&cell) && cell.testAttribute(Qt::WA_NativeWindow),
            "each group content cell owns an explicit native clipping boundary");
        QWidget nativePanel(&main); nativePanel.setObjectName("widgetEditInspector");
        const bool nativeBefore = nativePanel.testAttribute(Qt::WA_NativeWindow);
        check(!Unbound::setNativeGroupSurfaceClipping(&nativePanel) && nativePanel.testAttribute(Qt::WA_NativeWindow) == nativeBefore,
            "native clipping helper rejects original Resolve widgets without altering their rendering attributes");
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto layout=new QHBoxLayout(page); auto inspector=new QWidget(page); inspector->setObjectName("widgetEditInspector");
        inspector->setMinimumWidth(700); auto native=new QHBoxLayout(inspector); native->addStretch();
        auto right=new QPushButton("Rightmost native control",inspector); native->addWidget(right); layout->addWidget(inspector);
        main.resize(1000,500); main.show(); settle(); const auto policy=inspector->sizePolicy(); const bool fill=inspector->autoFillBackground();
        Unbound::MacPanelSession session;
        check(session.undock(inspector,page,"Inspector").isEmpty(),"narrow Inspector viewport detaches native content");
        session.window()->resize(300,360); settle();
        auto viewport=session.window()->findChild<QScrollArea *>("unboundPanelOverflow_widgetEditInspector");
        check(viewport && viewport->horizontalScrollBar()->maximum()>0 && session.window()->width()==300,
            "narrow Inspector scrolls instead of enlarging its floating host or clipping controls");
        check(inspector->minimumWidth()==700,"overflow does not relax native Inspector control constraints");
        auto rightHint=viewport->viewport()->findChild<QToolButton *>("openGateScrollRight");
        auto leftHint=viewport->viewport()->findChild<QToolButton *>("openGateScrollLeft");
        check(rightHint && rightHint->isVisible() && !rightHint->toolTip().isEmpty(),"narrow Inspector advertises hidden right-edge controls");
        if(rightHint)rightHint->click();settle();
        check(viewport->horizontalScrollBar()->value()>0 && leftHint && leftHint->isVisible(),"overflow hint scrolls without consuming panel height");
        viewport->horizontalScrollBar()->setValue(viewport->horizontalScrollBar()->minimum());settle();

        inspector->setParent(page); layout->addWidget(inspector); inspector->show(); settle(); settle();
        check(viewport && viewport->widget()==inspector && inspector->parentWidget()==viewport->viewport(),
            "native content reclamation recovers into the retained overflow viewport without duplicating the panel");
        check(native->geometry()==inspector->contentsRect(),
            "reclaimed Inspector control layout matches its resized content before pointer input");
        if (viewport) viewport->horizontalScrollBar()->setValue(viewport->horizontalScrollBar()->maximum());
        Unbound::activateNativeWindowForTest(session.window()); settle();
        const auto hitPoint=right->mapToGlobal(right->rect().center());
        auto hit=QApplication::widgetAt(hitPoint);
        if (hit!=right) {
            const auto expected=right->geometry(); const auto frame=session.window()->frameGeometry();
            std::fprintf(stderr,"Inspector hit diagnostic: point=%d,%d button=%d,%d,%d,%d host=%d,%d,%d,%d hit=%s/%s visible=%d scroll=%d/%d\n",
                hitPoint.x(),hitPoint.y(),expected.x(),expected.y(),expected.width(),expected.height(),
                frame.x(),frame.y(),frame.width(),frame.height(),hit ? hit->metaObject()->className() : "null",
                hit ? qPrintable(hit->objectName()) : "null",right->isVisible(),
                viewport ? viewport->horizontalScrollBar()->value() : -1,viewport ? viewport->horizontalScrollBar()->maximum() : -1);
            std::fprintf(stderr,"Inspector layout diagnostic: content=%dx%d layout=%dx%d viewport=%dx%d button-global-right=%d host-right=%d\n",
                inspector->width(),inspector->height(),native->geometry().width(),native->geometry().height(),
                viewport ? viewport->viewport()->width() : -1,viewport ? viewport->viewport()->height() : -1,
                right->mapToGlobal(right->rect().topRight()).x(),frame.right());
        }
        check(hit==right,
            "scrolling makes the original rightmost native button the real pointer target");
        auto neighbor=new QWidget(page); neighbor->setObjectName("OverflowNeighbor"); layout->addWidget(neighbor); neighbor->show(); settle();
        Unbound::MacPanelSession adjacent;
        check(adjacent.undock(neighbor,page,"Neighbor").isEmpty(),"overflow neighbor detaches");
        Unbound::PanelGroup group(nullptr);
        check(group.join(&session,&adjacent,Qt::Horizontal).isEmpty(),"overflow Inspector joins a magnetic group");
        group.window()->resize(620,400); settle();
        check(group.window()->width()==620 && viewport && viewport->horizontalScrollBar()->maximum()>0,
            "magnetic group can stay narrow while Inspector retains accessible overflow");
        const auto frame=group.window()->geometry();
        session.setHiddenByUser(true); group.syncVisibility(); settle();
        session.setHiddenByUser(false); group.syncVisibility(&session); settle();
        check(group.window()->geometry()==frame && viewport && viewport->widget()==inspector,
            "grouped door cycling retains the same overflow viewport and outer geometry");
        if (viewport) viewport->horizontalScrollBar()->setValue(viewport->horizontalScrollBar()->maximum());
        const auto scrollPosition=session.overflowOffset();
        check(session.beginGroupedExpansion(),"overflow Inspector can borrow a temporary maximize host");
        session.expandedWindow()->resize(1000,600); settle();
        session.endGroupedExpansion(); session.restoreOverflowOffset(scrollPosition); settle();
        check(session.overflowOffset()==scrollPosition && group.window()->geometry()==frame,
            "maximize restoration can restore overflow position without changing the group frame");
        check(session.dock().isEmpty(),"overflow viewport docks back to original native slot"); settle();
        check(inspector->parentWidget()==page && inspector->minimumWidth()==700 && inspector->sizePolicy()==policy && inspector->autoFillBackground()==fill &&
            !main.findChild<QScrollArea *>("unboundPanelOverflow_widgetEditInspector"),"native restoration removes the viewport and retains original constraints");
    }
    {
        QWidget nativeHost; QObject owner;
        QPointer<QWidget> a=new QWidget(&nativeHost), b=new QWidget(&nativeHost);
        Unbound::PanelLayoutCallbacks callbacks;
        callbacks.leaf=[&](const QString &id)->QWidget * { return id=="a" ? a.data() : id=="b" ? b.data() : nullptr; };
        callbacks.title=[](const QString &id) { return id.toUpper(); };
        const auto duplicate=Unbound::splitGroupTree({{"panel","a"}},{{"panel","a"}},Qt::Horizontal);
        check(!Unbound::buildPanelLayoutSurface(duplicate,&nativeHost,&owner,callbacks) && a && a->parentWidget()==&nativeHost,
            "invalid embedded surface cannot reparent or destroy native content");
        const auto tree=Unbound::splitGroupTree({{"panel","a"}},{{"panel","b"}},Qt::Orientation(0));
        auto surface=Unbound::buildPanelLayoutSurface(tree,&nativeHost,&owner,callbacks);
        auto tabs=qobject_cast<QTabWidget *>(surface);
        check(tabs && tabs->window()==&nativeHost && tabs->count()==2 && tabs->tabText(0)=="A",
            "split/tab renderer embeds in an ordinary native host without creating a floating window");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.show(); settle();
        Unbound::activateNativeWindowForTest(&main);
        const auto key = Unbound::nativeWindowNumber(&main);
        QElapsedTimer activation;activation.start();
        while(Unbound::nativeKeyWindowNumber()!=key && activation.elapsed()<1000)settle(10);
        check(key && Unbound::nativeKeyWindowNumber()==key,"group reserve fixture has acquired its intended key window before preparation");
        Unbound::GroupHostReserve reserve(&main); reserve.request(&main);
        QElapsedTimer readiness; readiness.start();
        while (!main.findChild<QWidget *>("unboundPreparedGroupHost") && readiness.elapsed()<1500) settle(10);
        QPointer<QWidget> prepared = main.findChild<QWidget *>("unboundPreparedGroupHost");
        if (!prepared) std::fprintf(stderr,"Reserve readiness timed out: buttons=%d grabber=%s popup=%s modal=%s\n",int(QApplication::mouseButtons()),
            qPrintable(QWidget::mouseGrabber() ? QWidget::mouseGrabber()->objectName() : QString()),
            qPrintable(QApplication::activePopupWidget() ? QApplication::activePopupWidget()->objectName() : QString()),
            qPrintable(QApplication::activeModalWidget() ? QApplication::activeModalWidget()->objectName() : QString()));
        check(prepared && !prepared->isVisible() && !(Unbound::nativeVisibilityFlags(prepared) & 1),
            "preparing an empty group never exposes another native window");
        if(Unbound::nativeKeyWindowNumber()!=key)std::fprintf(stderr,"Group reserve focus: expected=%lld actual=%lld prepared=%lld\n",(long long)key,(long long)Unbound::nativeKeyWindowNumber(),(long long)Unbound::nativeWindowNumber(prepared));
        check(Unbound::nativeKeyWindowNumber() == key,
            "preparing group chrome preserves the user's key window");
        auto taken = reserve.take(&main);
        check(taken == prepared && !taken->layout() && !taken->isVisible(),
            "group reservation transfers one empty hidden host without cached content");
        taken->setWindowTitle("Changed group membership");
        check(Unbound::nativeTitleTextForTest(taken) == "Changed group membership",
            "prepared group titles refresh without resizing or rebuilding native chrome");
        reserve.clear();
        check(prepared && taken->parentWidget() == &main,
            "clearing the reserve cannot destroy a host already adopted by a group");
        delete taken;
        reserve.request(&main); settle(300);
        QPointer<QWidget> unused = main.findChild<QWidget *>("unboundPreparedGroupHost");
        reserve.clear();
        check(unused.isNull(),"unused native group hosts are released on shutdown");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100,700);
        auto layout = new QVBoxLayout(&main);
        auto page = new QWidget(&main); layout->addWidget(page);
        auto nativeFooter = new QWidget(&main); nativeFooter->setObjectName("m_PageSwitcherContainer");
        (new QHBoxLayout(nativeFooter))->addWidget(new QLabel("Native page buttons", nativeFooter));
        layout->addWidget(nativeFooter); main.show(); settle();
        auto pageButton=new QPushButton("Edit",nativeFooter);pageButton->setObjectName("buttonConformPanel");nativeFooter->layout()->addWidget(pageButton);pageButton->show();settle();
        Unbound::FooterPanel footer(nullptr,[&] {return &main;},{});footer.wireMenu();
        check(nativeFooter->window()==&main,"surface footer keeps native page navigation in its original window");
        check(!nativeFooter->findChild<Unbound::SurfacePageNavigation *>("openGateSurfacePageNavigation")&&pageButton->isVisible(),"native footer keeps original page controls without an overlay");
        page->hide();settle();check(nativeFooter->isVisible(),"navigation visibility is independent of the current page");
        QMenu menu(&main);menu.setObjectName("workspaceShowPanel");footer.wireMenu();
        auto caller=menu.findChild<QAction *>("unboundPageNavigation");check(caller&&caller->isChecked(),"Panels caller reflects shared native visibility");
        if(caller)caller->trigger();settle();check(footer.snapshot()["hidden"].toBool(),"Panels caller hides global navigation");
        if(caller)caller->trigger();settle();check(!footer.snapshot()["hidden"].toBool(),"Panels caller restores global navigation");
        const auto saved=footer.snapshot();footer.refreshSurfaces();footer.setState(saved);settle();
        check(nativeFooter->parentWidget()==&main&&!footer.snapshot()["floating"].toBool(),"restore never transfers native footer ownership");
    }
    {
        QWidget main; main.resize(1400,700); auto outer = new QVBoxLayout(&main);
        auto page = new QWidget(&main); outer->addWidget(page); auto row = new QHBoxLayout(page);
        auto timeline = new QWidget(page); timeline->setObjectName("frameConformTimelines");
        auto inspector = new QWidget(page); inspector->setObjectName("widgetEditInspector");
        for (auto panel : {timeline, inspector}) {
            auto content = new QVBoxLayout(panel); content->addWidget(new QPushButton("Native control", panel)); row->addWidget(panel);
        }
        timeline->setMinimumWidth(900); inspector->setMinimumWidth(600); main.show(); settle();
        Unbound::MacPanelSession a, b;
        check(a.undock(timeline,page,"Timeline").isEmpty() && b.undock(inspector,page,"Inspector").isEmpty(),
            "wide native panels detach for narrow-group regression");
        a.window()->setWindowTitle("Project — Timeline");b.window()->setWindowTitle("Project — Inspector");
        Unbound::PanelGroup group(nullptr);
        check(group.join(&a,&b,Qt::Horizontal).isEmpty(), "wide native panels join one magnetic surface");
        for (auto label:group.window()->findChildren<QLabel *>("unboundGroupHeader"))
            check(!label->text().contains("Project") && (label->text()=="Timeline" || label->text()=="Inspector"),
                "group leaf labels use stable panel names instead of project-bearing window titles");
        group.window()->resize(620,400); settle();
        const auto frame = group.window()->geometry();
        auto splitter = group.window()->findChild<QSplitter *>();
        check(frame.width() >= 900 && splitter && timeline->minimumWidth() == 900, "Timeline preserves its native painting minimum instead of forcing a narrow outer viewport");
        if (splitter) { splitter->setSizes({1,1000}); settle(); }
        check(group.window()->geometry() == frame && timeline->width() >= 240 && inspector->width() >= 240,
            "minimum-boundary redistribution keeps outer group frame fixed and both panels usable");
        a.setHiddenByUser(true); group.syncVisibility(); settle();
        QToolButton *reveal = nullptr;
        for (auto candidate : group.window()->findChildren<QToolButton *>("unboundDetachPanel"))
            if (candidate->isVisible()) { reveal=candidate; break; }
        check(reveal && reveal->menu(),"hiding a group panel leaves a discoverable caller in its remaining header");
        if (reveal) {
            QMetaObject::invokeMethod(reveal->menu(),"aboutToShow",Qt::DirectConnection);
            QMenu *hidden=nullptr;
            for (auto action:reveal->menu()->actions()) if (action->menu() && action->text()=="Show Hidden Panels") hidden=action->menu();
            check(hidden && hidden->actions().size() == 1,"hidden-panel submenu lists precisely the hidden member");
            if (hidden && !hidden->actions().isEmpty()) hidden->actions().first()->trigger();
        }
        settle();
        check(!a.hiddenByUser() && group.contains(&a) && group.window()->geometry() == frame,
            "header caller restores the same member without detaching or changing outer frame");
        group.dissolve(true); settle();
        check(timeline->minimumWidth() == 900 && inspector->minimumWidth() == 600,
            "native docking restores both original width constraints");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1000,700);
        auto outer = new QVBoxLayout(&main);
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page);
        auto layout = new QVBoxLayout(page);
        auto stack = new QStackedWidget(page); stack->setObjectName("stackedEditSourceOfflinePlayers"); layout->addWidget(stack);
        auto empty = new QLabel("No clip loaded", stack); stack->addWidget(empty);
        auto player = new QWidget(stack); player->setObjectName("widgetEditSourcePlayer"); stack->addWidget(player);
        auto program = new QWidget(page); program->setObjectName("widgetEditTimelinePlayer"); layout->addWidget(program);
        auto singleViewer = new QAction("Single Viewer Mode", &main); singleViewer->setCheckable(true); singleViewer->setEnabled(false);
        singleViewer->setShortcut(QKeySequence(Qt::Key_F9));
        stack->setCurrentWidget(empty); main.show(); settle();
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle();
            controls.findChild<QPushButton *>("undock_widgetEditSourcePlayer")->click(); settle();
            check(stack->window() != &main && stack->window()->objectName() == "unboundFloating_widgetEditSourcePlayer",
                "Source Undock works with the offline surface and no clip loaded");
            check(stack->currentWidget() == empty && !player->isVisible(),
                "empty Source detachment preserves native offline selection without loading footage");
            controls.findChild<QPushButton *>("undock_widgetEditTimelinePlayer")->click(); settle();
            controls.findChild<QComboBox *>("groupFirst")->setCurrentIndex(2);
            controls.findChild<QComboBox *>("groupSecond")->setCurrentIndex(3);
            controls.findChild<QPushButton *>("group_1")->click(); settle(200);
            check(singleViewer->isEnabled(), "native viewer mode command remains available for managed viewers");
            const auto viewerFrame = stack->window()->geometry();
            singleViewer->trigger(); settle();
            check(!empty->isVisible() && program->isVisible(), "native single-viewer command hides Source while retaining Program");
            singleViewer->trigger(); settle();
            check(empty->isVisible() && program->isVisible() && singleViewer->shortcut() == QKeySequence(Qt::Key_F9),
                "native dual-viewer command restores Source and preserves a non-default key mapping");
            for (int cycle=0; cycle<20; ++cycle) { singleViewer->trigger(); settle(); singleViewer->trigger(); settle(); }
            check(stack->window()->geometry() == viewerFrame,
                "twenty native viewer-command cycles preserve the outer magnetic window frame");
            controls.findChild<QPushButton *>("dock_widgetEditSourcePlayer")->click(); settle();
            check(stack->parentWidget() == page && layout->indexOf(stack) >= 0 && stack->currentWidget() == empty,
                "empty Source docking restores its original slot and offline state");
            stack->hide(); singleViewer->setChecked(true); singleViewer->setEnabled(false);
            controls.findChild<QPushButton *>("undock_widgetEditSourcePlayer")->click(); settle(220);
            check(stack->window() != &main && stack->isVisible() && stack->currentWidget() == empty,
                "hidden offline Source floats without footage or an enabled native dual-viewer command");
            controls.findChild<QPushButton *>("dock_widgetEditSourcePlayer")->click(); settle();
            check(stack->parentWidget() == page && stack->isHidden(),
                "docking a previously hidden Source restores its original native visibility");
        }
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1000,700);
        auto outer = new QVBoxLayout(&main); auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page);
        auto layout = new QVBoxLayout(page);
        auto timelineToolbar = new QWidget(page); timelineToolbar->setObjectName("frameEditToolbarTop"); layout->addWidget(timelineToolbar);
        (new QHBoxLayout(timelineToolbar))->addWidget(new QLabel("Native timeline tools", timelineToolbar));
        auto toolbarModes = new QStackedWidget(page); layout->addWidget(toolbarModes);
        auto mode = new QWidget; mode->setObjectName("pageSingleScreenModeTools"); toolbarModes->addWidget(mode);
        auto modeLayout = new QHBoxLayout(mode); auto left = new QWidget(mode); left->setObjectName("frameEditTopBarLeftInSSM"); modeLayout->addWidget(left);
        (new QHBoxLayout(left))->addWidget(new QLabel("Media Pool / Index / Inspector", left));
        QMenu nativeMenu(&main); nativeMenu.setObjectName("workspaceShowPanel");
        QMenuBar menuBar(&main); QMenu nativeWorkspace(&menuBar); nativeWorkspace.setObjectName("workspace"); menuBar.addMenu(&nativeWorkspace); main.show(); settle();
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle(400);
            auto timelineFloat = timelineToolbar->findChild<QToolButton *>("unboundUndockToolbar_frameEditToolbarTop");
            check(!timelineFloat, "attached Timeline toolbar has no independent float control");
            auto workspaceFloat = mode->findChild<QToolButton *>("unboundUndockToolbar_unboundWorkspaceToolbar");
            check(!timelineFloat && !workspaceFloat, "Timeline tools remain attached and Workspace Toolbar remains pinned");
            if (timelineFloat) timelineFloat->click();settle(250);
            check(timelineToolbar->window()==&main && toolbarModes->window()==&main,
                "Timeline toolbar and Workspace Toolbar retain their native owners");
            check(toolbarModes->currentWidget()==mode,"pinned workspace header preserves native mode selection");
            bool timelineCaller = false, workspaceCaller = false;
            for (auto action : nativeMenu.actions()) {
                timelineCaller |= action->text() == "Timeline Toolbar";
                workspaceCaller |= action->text() == "Workspace Toolbar";
            }
            check(timelineCaller && workspaceCaller, "both toolbars remain discoverable in Show Panel in Workspace");
            auto nativeRecall = menuBar.findChild<QAction *>("unboundRestoreNativeWorkspace");
            check(nativeRecall && nativeRecall->text() == "Use Original Resolve UI", "permanent native-workspace recall is prepopulated in Workspace UI");
            controls.hide();
            if (nativeRecall) nativeRecall->trigger(); settle(200);
            check(timelineToolbar->parentWidget() == page && toolbarModes->parentWidget() == page,
                "top native-workspace recovery restores both native toolbar slots");
            check(!controls.findChild<QCheckBox *>("hideResolveBackground")->isChecked() && (Unbound::nativeVisibilityFlags(&main) & 1),
                "permanent recovery reveals the native background and exits floating workspace mode");
        }
        {
            PanelControls reopened; reopened.show(); settle(400);
            auto button = timelineToolbar->findChild<QToolButton *>("unboundUndockToolbar_frameEditToolbarTop");
            if (button) button->click(); settle(200);
            check(!button && timelineToolbar->window() == &main,
                "reopening does not restore a forbidden independent Timeline toolbar control");
            reopened.findChild<QPushButton *>("bulk_1")->click(); settle();
        }
    }
    {
        QWidget main; main.resize(1000,700); auto outer = new QVBoxLayout(&main);
        auto page = new QWidget(&main); outer->addWidget(page); auto row = new QHBoxLayout(page);
        QList<Unbound::MacPanelSession *> sessions;
        for (const auto &name : {"Source", "Program", "MediaPool", "Inspector"}) {
            auto content = new QWidget(page); content->setObjectName(name); row->addWidget(content);
            sessions.append(new Unbound::MacPanelSession(&main));
        }
        main.show(); settle();
        for (int index = 0; index < sessions.size(); ++index)
            check(sessions[index]->undock(row->itemAt(index)->widget(), page, row->itemAt(index)->widget()->objectName()).isEmpty(), "prepare section member");
        const auto leftTree = Unbound::splitGroupTree({{"panel","Source"}}, {{"panel","Program"}}, Qt::Horizontal);
        const auto rightTree = Unbound::splitGroupTree({{"panel","MediaPool"}}, {{"panel","Inspector"}}, Qt::Vertical);
        Unbound::PanelGroup group(nullptr);
        check(group.initialize(sessions, Unbound::splitGroupTree(leftTree,rightTree,Qt::Orientation(0)), QRect(100,100,800,500)).isEmpty(),
            "nested tab fixture contains split arrangements rather than only individual panels"); settle();
        auto tabs = group.window()->findChild<QTabWidget *>("unboundPanelTabs");
        auto detached = tabs ? group.detachSection(tabs->widget(0)) : nullptr; settle();
        check(detached && sessions[0]->window()->window() == sessions[1]->window()->window() &&
            sessions[2]->window()->window() == sessions[3]->window()->window() &&
            sessions[0]->window()->window() != sessions[2]->window()->window(),
            "tearing off a section preserves both donor and detached arrangements");
        if (detached) {
            check(detached->snapshot()["tree"].toObject()["orientation"] == "horizontal",
                "section tear-off preserves internal orientation");
            check(detached->merge(&group, Qt::Vertical).isEmpty(), "a detached section can merge back into another arrangement");
            QStringList names;
            check(Unbound::groupTreeNames(detached->snapshot()["tree"].toObject(), names) && names.size() == 4,
                "section remerge neither loses nor duplicates panels");
            delete detached;
        }
    }
    for (auto orientation : {Qt::Horizontal, Qt::Vertical}) for (bool contents : {false, true}) {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100, 700);
        auto mainLayout = new QVBoxLayout(&main);
        auto page = new QWidget(&main); mainLayout->addWidget(page);
        auto row = new QHBoxLayout(page);
        auto source = new QWidget(page); source->setObjectName("widgetEditSourcePlayer");
        auto program = new QWidget(page); program->setObjectName("widgetEditTimelinePlayer");
        auto sourceLayout = new QVBoxLayout(source); sourceLayout->addWidget(new QLabel("Source", source));
        auto programLayout = new QVBoxLayout(program); programLayout->addWidget(new QLabel("Program", program));
        row->addWidget(source); row->addWidget(program); main.show(); settle();
        Unbound::MacPanelSession a, b;
        check((contents ? a.undockContents(source, page, "Source") : a.undock(source, page, "Source")).isEmpty(), "float source");
        check((contents ? b.undockContents(program, page, "Program") : b.undock(program, page, "Program")).isEmpty(), "float program");
        settle();
        const auto beforeA = a.floatingGeometry(), beforeB = b.floatingGeometry();
        Unbound::PanelGroup group(nullptr);
        check(group.join(&a, &b, orientation).isEmpty(), "join pair"); settle();
        check(a.grouped() && b.grouped() && !a.window()->isWindow() && !b.window()->isWindow(), "members cease to be independent top-level windows");
        check(a.window()->window() == group.window() && b.window()->window() == group.window(), "both members share exactly one native host");
        auto splitter = group.window()->findChild<QSplitter *>();
        check(splitter && splitter->count() == 2 && splitter->orientation() == orientation, "group uses native split layout in requested direction");
        const auto startA = a.window()->mapToGlobal(QPoint()), startB = b.window()->mapToGlobal(QPoint());
        group.window()->move(group.window()->pos() + QPoint(15, 20));
        check(a.window()->mapToGlobal(QPoint()) - startA == QPoint(15, 20) &&
            b.window()->mapToGlobal(QPoint()) - startB == QPoint(15, 20), "one move updates both panels synchronously without follower timer");
        splitter->setSizes({250, 400}); settle();
        const auto saved = group.snapshot();
        check(saved["sizes"].toArray().size() == 2, "split sizes are serializable");
        page->hide(); settle(); check(!group.window()->isVisible(), "group hides when Edit leaves");
        page->show(); settle(); check(group.window()->isVisible() && a.window()->isVisible() && b.window()->isVisible(), "group and both panels return with Edit");
        a.setHiddenByUser(true); b.setHiddenByUser(true); settle();
        check(!group.window()->isVisible(), "fully hidden group stays hidden");
        a.setHiddenByUser(false); settle(); check(group.window()->isVisible(), "showing a member recalls its group");
        group.dissolve(); settle();
        check(!a.grouped() && !b.grouped() && a.window()->isWindow() && b.window()->isWindow(), "ungroup restores independent windows");
        check(a.floatingGeometry() == beforeA && b.floatingGeometry() == beforeB, "ungroup restores standalone geometries");
        b.setHiddenByUser(false);
        check(group.join(&a, &b, orientation).isEmpty(), "regroup"); group.restore(saved); settle();
        check(group.snapshot()["sizes"] == saved["sizes"], "group restores saved split ratio");
        QLabel *header = nullptr;
        for (auto candidate : group.window()->findChildren<QLabel *>("unboundGroupHeader"))
            if (candidate->property("groupMember").toInt() == 0) header = candidate;
        const QPoint press = header->mapToGlobal(QPoint(12, 7));
        const QPoint anchor = press - header->parentWidget()->parentWidget()->mapToGlobal(QPoint());
        QMouseEvent down(QEvent::MouseButtonPress, QPointF(12, 7), QPointF(press), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(header, &down);
        const QPoint moved = press + QPoint(70, 45);
        QMouseEvent move(QEvent::MouseMove, QPointF(82, 52), QPointF(moved), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(header, &move);
        check(!group.active() && !a.grouped() && !b.grouped(), "dragging a member header tears off its group");
        check(a.window()->frameGeometry().topLeft() == moved - anchor, "tear-off preserves pointer-to-panel offset without delayed movement");
        const QPoint released = moved + QPoint(20, 10);
        QMouseEvent up(QEvent::MouseButtonRelease, QPointF(12, 7), QPointF(released), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(a.window(), &up);
        check(a.window()->frameGeometry().topLeft() == released - anchor, "detached window follows pointer synchronously through release");
        check(group.join(&a, &b, orientation).isEmpty(), "regroup after drag tear-off");
        check(a.dock().isEmpty(), "docking one member safely releases group"); settle();
        check(!group.active() && !a.active() && b.active() && !b.grouped(), "neighbor remains a usable floating panel");
        check(b.dock().isEmpty(), "dock remaining panel"); settle();
        check(source->layout() == sourceLayout && program->layout() == programLayout && row->count() == 2,
            "both original Resolve slots and layouts survive grouping and docking");
        check((contents ? a.undockContents(source, page, "Source") : a.undock(source, page, "Source")).isEmpty(), "refloat source for close test");
        check((contents ? b.undockContents(program, page, "Program") : b.undock(program, page, "Program")).isEmpty(), "refloat program for close test");
        check(group.join(&a, &b, orientation).isEmpty(), "regroup for close test");
        auto retainedHost=group.window();const auto retainedFrame=retainedHost->geometry();const auto retainedTree=group.snapshot().value("tree");
        retainedHost->close(); settle();
        check(group.active() && a.active() && b.active() && group.windowHidden() && !retainedHost->isVisible(), "closing group retains both members and hides its host");
        group.setWindowHidden(false);settle();
        check(group.window()==retainedHost && retainedHost->geometry()==retainedFrame && group.snapshot()["tree"]==retainedTree, "group recall preserves host geometry and tree");
        group.dissolve(true);
        check(!a.active()&&!b.active(),"explicit group docking still releases members");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100, 700);
        auto page = new QWidget(&main); auto mainLayout = new QVBoxLayout(&main); mainLayout->addWidget(page);
        auto row = new QHBoxLayout(page);
        QList<QWidget *> widgets;
        QList<Unbound::MacPanelSession *> sessions;
        const QStringList names = {"Inspector", "Timeline", "Source", "Program", "MediaPool"};
        for (const auto &name : names) {
            auto widget = new QLabel(name, page); widget->setObjectName(name); row->addWidget(widget); widgets.append(widget);
            sessions.append(new Unbound::MacPanelSession(&main));
        }
        main.show(); settle();
        for (int i = 0; i < widgets.size(); ++i) check(sessions[i]->undock(widgets[i], page, names[i]).isEmpty(), "multi-panel setup");
        Unbound::PanelGroup left(nullptr), right(nullptr);
        check(left.join(sessions[0], sessions[1], Qt::Horizontal).isEmpty() && right.join(sessions[2], sessions[3], Qt::Vertical).isEmpty(), "create two independent groups");
        check(left.merge(&right, Qt::Horizontal).isEmpty(), "merge two groups without redocking members"); settle();
        check(left.active() && !right.active() && left.snapshot()["panels"].toArray().size() == 4, "merged group retains four panel identities");
        for (int i = 0; i < 4; ++i) check(sessions[i]->window()->window() == left.window(), "all merged panels share one native window");
        check(left.add(sessions[4], Qt::Vertical, true, "Timeline").isEmpty(), "insert fifth panel beside a specific leaf"); settle();
        QLabel *transferredHeader = nullptr;
        for (auto header : left.window()->findChildren<QLabel *>("unboundGroupHeader")) if (header->property("groupPanel") == "Program") transferredHeader = header;
        const auto press = transferredHeader->mapToGlobal(QPoint(10, 7));
        QMouseEvent down(QEvent::MouseButtonPress, QPointF(10, 7), QPointF(press), Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
        QApplication::sendEvent(transferredHeader, &down);
        QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(press + QPoint(80, 40)), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
        QApplication::sendEvent(transferredHeader, &move);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(press + QPoint(80, 40)), Qt::LeftButton, Qt::NoButton, Qt::AltModifier);
        QApplication::sendEvent(&main, &release); settle();
        check(!sessions[3]->grouped() && left.active() && left.snapshot()["panels"].toArray().size() == 4,
            "a member transferred from another group retains functional tear-off handlers");
        check(left.add(sessions[3], Qt::Horizontal, false, "Source").isEmpty(), "reattach transferred member without dismantling the remainder");
        auto saved = left.snapshot(); QStringList leaves;
        check(Unbound::groupTreeNames(saved["tree"].toObject(), leaves) && leaves.size() == 5, "nested split tree serializes every panel once");
        check(sessions[1]->dock().isEmpty(), "dock one leaf from a larger group"); settle();
        check(left.active() && left.snapshot()["panels"].toArray().size() == 4 && !sessions[1]->active(), "removing one member preserves the remaining group");
        check(widgets[1]->parentWidget() == page, "removed leaf restores its original Resolve slot");
        saved = left.snapshot(); left.dissolve(); settle();
        QList<Unbound::MacPanelSession *> remaining{sessions[0], sessions[2], sessions[3], sessions[4]};
        Unbound::PanelGroup restored(nullptr);
        check(restored.initialize(remaining, saved["tree"].toObject(), Unbound::WorkspaceSettings::rectangle(saved)).isEmpty() && restored.restore(saved), "restore nested arrangement from saved tree");
        settle(); leaves.clear();
        check(Unbound::groupTreeNames(restored.snapshot()["tree"].toObject(), leaves) && leaves.size() == 4 && !leaves.contains("Timeline"), "restored tree collapses removed/unary branches without losing other leaves");
        auto invalid = Unbound::splitGroupTree({{"panel", "Inspector"}}, {{"panel", "Inspector"}}, Qt::Horizontal); leaves.clear();
        check(!Unbound::groupTreeNames(invalid, leaves), "duplicate saved panel identities are rejected");
        restored.window()->close(); settle();
        check(restored.active()&&restored.windowHidden(),"closing nested group retains its tree");
        for (auto session : remaining) check(session->active()&&session->grouped(), "closed nested group retains every remaining member");
        restored.dissolve(true);
        for (auto session : sessions) check(!session->active(), "explicit nested group docking releases every member");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100, 700);
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform");
        auto mainLayout = new QVBoxLayout(&main); mainLayout->addWidget(page); auto row = new QHBoxLayout(page);
        QList<QLabel *> content;
        for (const auto &name : {"frameConformTimelines", "widgetEditSourcePlayer", "widgetEditTimelinePlayer", "frameEditPageMediaPool"}) {
            auto widget = new QWidget(page); widget->setObjectName(name); row->addWidget(widget);
            auto label = new QLabel(name, widget); auto layout = new QVBoxLayout(widget); layout->addWidget(label); content.append(label);
        }
        main.show(); settle();
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle();
            const auto combine = [&](int first, int second, int direction) {
                controls.findChild<QComboBox *>("groupFirst")->setCurrentIndex(first);
                controls.findChild<QComboBox *>("groupSecond")->setCurrentIndex(second);
                controls.findChild<QPushButton *>(QStringLiteral("group_%1").arg(direction))->click(); settle(200);
            };
            combine(4, 2, 1); combine(1, 3, 0);
            auto moving = static_cast<Unbound::MacFloatingHost *>(content[1]->window());
            QWidget *targetCell = content[0];
            while (targetCell && !targetCell->objectName().startsWith("unboundGroupCell_")) targetCell = targetCell->parentWidget();
            check(targetCell != nullptr, "group drag finds an individual destination leaf");
            const auto destination = targetCell->parentWidget();
            const QPoint pointer = destination->mapToGlobal(QPoint(destination->width() / 2, 1));
            const auto press = moving->mapToGlobal(QPoint(100, 7));
            moving->beginPointerDrag(press);
            QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(pointer), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(moving, &move);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(pointer), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(&main, &release); settle(250);
            for (auto label : content) check(label->window() == content.first()->window(), "controls merge two saved pairs into one four-panel workspace");
            content.first()->window()->move(20000, -20000);
            controls.findChild<QPushButton *>("center_widgetEditSourcePlayer")->click(); settle();
            auto screen = QGuiApplication::screenAt(QCursor::pos()); if (!screen) screen = QGuiApplication::primaryScreen();
            check(screen->availableGeometry().contains(content.first()->window()->frameGeometry()), "Center rescues a complete nested group without dissolving it");
            Unbound::WorkspaceSettings saved;
            const auto layouts = saved.data["groups"].toArray();
            check(layouts.size() == 1 && layouts.first().toObject()["panels"].toArray().size() == 4 && layouts.first().toObject()["tree"].isObject(),
                "workspace persistence records nested group instead of flattening it into pairs");
        }
        settle();
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle(650);
            for (auto label : content) check(label->window() == content.first()->window() && label->window() != &main,
                "reloaded workspace restores all four panels into the same native group");
            controls.findChild<QPushButton *>("dock_widgetEditSourcePlayer")->click(); settle();
            check(content[1]->window() == &main && content[0]->window() == content[2]->window() && content[2]->window() == content[3]->window(),
                "docking one member leaves the other three grouped");
            controls.findChild<QPushButton *>("bulk_1")->click(); settle();
            for (auto label : content) check(label->window() == &main, "Dock All restores a mixed nested workspace");
        }
    }
    {
        QWidget main; main.setObjectName("UiMainWindow");
        auto page = new QWidget(&main); auto ownerLayout = new QVBoxLayout(&main); ownerLayout->addWidget(page);
        auto layout = new QHBoxLayout(page);
        auto inspector = new QWidget(page), media = new QWidget(page), source = new QWidget(page);
        inspector->setObjectName("Inspector"); media->setObjectName("MediaPool"); source->setObjectName("Source");
        for (auto widget : {inspector, media, source}) { layout->addWidget(widget); auto inner = new QVBoxLayout(widget); inner->addWidget(new QPushButton("Native control", widget)); }
        main.show(); settle();
        Unbound::MacPanelSession i, m, s;
        check(i.undock(inspector, page, "Inspector").isEmpty() && m.undock(media, page, "Media Pool").isEmpty() && s.undock(source, page, "Source").isEmpty(), "tab fixture setup");
        Unbound::PanelGroup group(nullptr);
        check(group.join(&i, &m, Qt::Orientation(0)).isEmpty(), "center placement creates real native Qt tabs"); settle();
        auto tabs = group.window()->findChild<QTabWidget *>("unboundPanelTabs");
        check(tabs && tabs->count() == 2, "tab stack contains both real panel surfaces");
        check(inspector->isVisible() && !media->isVisible(),
            "a newly built tab stack exposes only its selected native panel");
        tabs->setCurrentIndex(1); settle();
        check(media->isVisible() && !inspector->isVisible(),"switching tabs hides the previous native surface");
        group.syncVisibility(&i); settle();
        check(group.selected(&i) && !group.selected(&m), "revealing a covered panel selects its tab");
        Unbound::activateNativeWindowForTest(group.window()); settle();
        auto visibleButton = inspector->findChild<QPushButton *>();
        check(visibleButton && QApplication::widgetAt(visibleButton->mapToGlobal(visibleButton->rect().center())) == visibleButton,
            "selected tab's native button remains the actual pointer hit target");
        i.setHiddenByUser(true); group.syncVisibility(); settle();
        check(!tabs->isTabVisible(0) && tabs->isTabVisible(1) && group.selected(&m), "hiding a tab leaves its neighbor usable");
        i.setHiddenByUser(false); group.syncVisibility(&i); settle();
        check(tabs->isTabVisible(0) && group.selected(&i), "door restores and selects the panel's remembered tab");
        auto host = static_cast<Unbound::MacFloatingHost *>(group.window());
        host->setPinned(true); check(group.snapshot()["pinned"].toBool(), "group pin is persisted on its shared native host");
        const auto pinnedFrame=host->geometry();
        for (int cycle=0;cycle<4;++cycle) {
            i.setHiddenByUser(true); group.syncVisibility(); settle();
            check(group.selected(&m) && host->pinned(),"pinned group can close Inspector door without unpinning");
            i.setHiddenByUser(false); group.syncVisibility(&i); settle();
            check(group.selected(&i) && host->pinned() && host->geometry()==pinnedFrame,
                "pinned group restores the door without changing pin or outer frame");
        }
        const auto before = group.snapshot();
        auto surface = i.window(); auto nativeLayout = surface->layout();
        check(i.beginGroupedExpansion() && i.expandedWindow() && !surface->layout(), "temporary maximize borrows layout without releasing group slot");
        i.expandedWindow()->setGeometry(20, 20, 800, 600); settle();
        i.endGroupedExpansion(); settle();
        check(i.grouped() && i.window() == surface && surface->layout() == nativeLayout && !i.expandedWindow(), "maximize restore returns native content to the exact same tab slot");
        check(group.snapshot()["tree"] == before["tree"], "temporary maximize preserves tab selection and group structure");
        check(group.add(&s, Qt::Vertical, false).isEmpty(), "source can span beneath a tab stack"); settle();
        check(inspector->isVisible() && !media->isVisible() && source->isVisible(),
            "rebuilding a tabbed subtree cannot expose its inactive native panel over the selected page");
        group.dissolve(); settle();
        auto top = Unbound::splitGroupTree({{"panel", "MediaPool"}}, {{"panel", "Inspector"}}, Qt::Horizontal);
        const auto innerTabs=Unbound::splitGroupTree({{"panel","MediaPool"}},{{"panel","Inspector"}},Qt::Orientation(0));
        const auto nestedTabs=Unbound::splitGroupTree({{"panel","Source"}},innerTabs,Qt::Orientation(0));
        check(group.initialize({&i,&m,&s},nestedTabs,QRect(50,50,900,700)).isEmpty(),"nested tab-door arrangement initializes"); settle();
        const auto nestedFrame=group.window()->geometry();
        check(group.selected(&s) && !group.selected(&i),"Inspector begins covered behind two tab levels");
        group.syncVisibility(&i); settle();
        check(group.selected(&i) && inspector->isVisible() && !source->isVisible(),
            "revealing Inspector selects every enclosing tab without flattening the arrangement");
        i.setHiddenByUser(true); group.syncVisibility(); settle();
        check(group.selected(&s) && source->isVisible(),"closing a nested door returns to the previously selected outer panel");
        i.setHiddenByUser(false); group.syncVisibility(&i); settle();
        check(group.selected(&i) && group.window()->geometry()==nestedFrame,"nested door restores its place without resizing the outer surface");
        group.syncVisibility(&s); settle();
        i.setHiddenByUser(true); group.syncVisibility(); settle();
        check(group.selected(&s),"hiding a covered nested panel does not change the selected outer tab");
        i.setHiddenByUser(false); group.syncVisibility(&i); settle();
        group.dissolve(); settle();
        auto puzzle = Unbound::splitGroupTree(top, {{"panel", "Source"}}, Qt::Vertical);
        check(group.initialize({&i, &m, &s}, puzzle, QRect(50, 50, 900, 700)).isEmpty(), "screenshot arrangement has a side-by-side top row and spanning source below"); settle();
        auto leftColumn = Unbound::splitGroupTree({{"panel", "Inspector"}}, {{"panel", "Source"}}, Qt::Vertical);
        group.dissolve(); settle();
        check(group.initialize({&i,&m,&s}, Unbound::splitGroupTree(leftColumn, {{"panel","MediaPool"}}, Qt::Horizontal), QRect(50,50,900,700)).isEmpty(),
            "reported arrangement has Inspector above Source and Media Pool spanning the right"); settle();
        auto outerSplit = group.window()->findChild<QSplitter *>();
        auto column = outerSplit ? qobject_cast<QSplitter *>(outerSplit->widget(0)) : nullptr;
        if (column) {
            const auto before = column->sizes(); const auto mediaGeometry = m.window()->geometry();
            auto handle = column->handle(1); const QPoint downPoint = handle->rect().center();
            const QPoint global = handle->mapToGlobal(downPoint);
            QMouseEvent down(QEvent::MouseButtonPress, QPointF(downPoint), QPointF(global), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(handle, &down);
            QMouseEvent move(QEvent::MouseMove, QPointF(downPoint-QPoint(0,60)), QPointF(global-QPoint(0,60)), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(handle, &move);
            QMouseEvent up(QEvent::MouseButtonRelease, QPointF(downPoint-QPoint(0,60)), QPointF(global-QPoint(0,60)), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(handle, &up); settle();
            check(column->sizes()[0] < before[0] && m.window()->geometry() == mediaGeometry,
                "dragging Source's top boundary resizes its own column without moving Media Pool");
            const auto openColumn = column->sizes(), openOuter = outerSplit->sizes();
            const auto outerFrame = group.window()->geometry();
            for (int cycle=0; cycle<20; ++cycle) {
                i.setHiddenByUser(true); group.syncVisibility(); settle();
                check(group.window()->geometry() == outerFrame, "closing a nested door preserves outer frame");
                i.setHiddenByUser(false); group.syncVisibility(&i); settle();
                check(group.window()->geometry() == outerFrame, "opening a nested door preserves outer frame");
            }
            check(column->sizes() == openColumn && outerSplit->sizes() == openOuter,
                "nested door cycling preserves user-dragged dividers and neighboring column widths");
        } else check(false, "reported layout exposes its vertical column splitter");
        const bool inspectorDoor=i.hiddenByUser(),mediaDoor=m.hiddenByUser(),sourceDoor=s.hiddenByUser();
        group.setWindowHidden(true);settle();
        check(!group.window()->isVisible() && group.snapshot()["hidden"].toBool(),"whole-group door hides its window and persists separately");
        group.setWindowHidden(false);settle();
        check(group.window()->isVisible() && i.hiddenByUser()==inspectorDoor && m.hiddenByUser()==mediaDoor && s.hiddenByUser()==sourceDoor,
            "whole-group door preserves every internal panel door");
        auto beforeHide = group.snapshot(); i.setHiddenByUser(true); group.syncVisibility(); settle();
        check(!i.window()->parentWidget()->isVisible() && m.window()->isVisible(), "split door collapses the wrapper rather than leaving empty content space");
        i.setHiddenByUser(false); group.syncVisibility(&i); settle();
        check(i.window()->isVisible() && group.snapshot()["panels"].toArray().size() == 3, "split door keeps every panel identity through hide/show");
        check(group.snapshot()["tree"].toObject()["orientation"] == beforeHide["tree"].toObject()["orientation"],
            "split visibility never converts a side-by-side arrangement into a vertical stack");
        group.window()->close(); settle();
    }
    {
        qputenv("UNBOUND_WORKSPACE_FILE", (temporary.path() + "/native-door-workspace.json").toUtf8());
        QWidget main; main.setObjectName("UiMainWindow");
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform");
        auto ownerLayout = new QVBoxLayout(&main); ownerLayout->addWidget(page);
        auto layout = new QVBoxLayout(page);
        auto toggle = new QPushButton("Inspector", page); toggle->setObjectName("btnEditToggleInspectorInSSM");
        toggle->setCheckable(true); toggle->setChecked(true); layout->addWidget(toggle);
        auto inspector = new QWidget(page); inspector->setObjectName("widgetEditInspector"); layout->addWidget(inspector);
        auto media = new QWidget(page); media->setObjectName("frameEditPageMediaPool"); layout->addWidget(media);
        auto inner = new QVBoxLayout(media); inner->addWidget(new QLabel("Native Media Pool", media));
        main.show(); settle();
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle();
            controls.findChild<QPushButton *>("undock_widgetEditInspector")->click(); settle(100);
            toggle->click(); settle(120);
            check(!inspector->window()->isVisible() && !toggle->isChecked(), "native click hides floating Inspector once despite toggled and clicked signals");
            toggle->setChecked(true); settle(120);
            check(!inspector->window()->isVisible() && !toggle->isChecked(), "programmatic native check observation cannot recall a hidden Inspector");
            toggle->click(); settle(120);
            check(inspector->window()->isVisible() && toggle->isChecked(), "explicit native activation recalls Inspector through its retained session");
            controls.findChild<QComboBox *>("groupFirst")->setCurrentIndex(0);
            controls.findChild<QComboBox *>("groupSecond")->setCurrentIndex(4);
            controls.findChild<QPushButton *>("group_3")->click(); settle(240);
            auto tabs = inspector->window()->findChild<QTabWidget *>("unboundPanelTabs");
            check(tabs && tabs->count() == 2, "controls provide explicit tab arrangement entry point");
            if (tabs) {
                tabs->setCurrentIndex(1); toggle->click(); settle(120);
                check(tabs->currentIndex() == 0 && toggle->isChecked(), "native Inspector toggle first selects a covered tab instead of closing it");
                toggle->click(); settle(120);
                check(!tabs->isTabVisible(0) && tabs->isTabVisible(1), "next native toggle closes only Inspector's door");
                toggle->click(); settle(120);
                check(tabs->isTabVisible(0) && tabs->currentIndex() == 0, "native toggle returns Inspector to its existing tab");
            }
            controls.findChild<QPushButton *>("bulk_1")->click(); settle();
        }
    }
    if (!failures) std::puts("PASS: nested single-host groups, group merge, leaf insertion/removal, persisted control reload, drag release and original-slot docking");
    return failures ? 1 : 0;
}
