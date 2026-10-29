// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "controlscroll.h"
#include "floatinghost.h"
#include "viewerpreparation.h"
#include "tabtearoff.h"
#include "workspacecallers.h"
#include "groupvisibility.h"
#include "panelwidth.h"
#include "panelmenus.h"
#include "viewermodecommand.h"
int main(int argc, char **argv)
{
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");
    const auto foreground = Unbound::nativeFrontmostProcessForTest();
    QApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR", logs.path().toUtf8());
    int failures = 0;
    const auto check = [&](bool pass, const char *message) { if (!pass) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); } };
    {
        QWidget moving; moving.setAttribute(Qt::WA_DontShowOnScreen); moving.setObjectName("unboundFloating_geometryFixture");
        Unbound::WindowDiagnostics geometry(nullptr); qApp->installEventFilter(&geometry);
        for (int i = 0; i < 100; ++i) { QMoveEvent move(QPoint(i,0),QPoint(i-1,0)); QApplication::sendEvent(&moving,&move); }
        geometry.flushGeometry(); qApp->removeEventFilter(&geometry);
        Unbound::flushDiagnostics();
        QFile events(logs.path() + "/events.jsonl"); events.open(QIODevice::ReadOnly);
        int records = 0, changes = 0;
        while (!events.atEnd()) {
            const auto record = QJsonDocument::fromJson(events.readLine()).object();
            if (record["name"] == moving.objectName()) { ++records; changes += record["coalesced_events"].toInt(); }
        }
        check(records == 1 && changes == 100, "drag geometry diagnostics coalesce hot-path writes while retaining the event count");
    }
    {
        QWidget timeline; timeline.setObjectName("frameConformTimelines");
        QWidget toolbar(&timeline); toolbar.setObjectName("frameEditToolbarTop");
        QWidget mixer(&timeline); mixer.setObjectName("frameEditAudioMeter");
        QPushButton timelineOptions(&toolbar); timelineOptions.setObjectName("btnEditToolbarPreferences");
        QPushButton mixerOptions(&mixer); mixerOptions.setObjectName("btnAudioMixerOptions");
        QMenu timelineMenu(&timelineOptions), mixerMenu(&mixerOptions), unrelatedMenu(&mixer);
        int timelineCalls = 0, mixerCalls = 0, toolbarCalls = 0;
        const auto bind = [&](Unbound::PanelMenus &menus) {
            menus.add({"frameConformTimelines", [] { return false; }, [&] { ++timelineCalls; }});
            menus.add({"frameEditAudioMeter", [] { return true; }, [&] { ++mixerCalls; }});
            menus.add({"frameEditToolbarTop", [] { return false; }, [&] { ++toolbarCalls; }});
        };
        const auto prepare = [](QMenu &menu) { QEvent show(QEvent::Show); QApplication::sendEvent(&menu, &show); };
        {
            Unbound::PanelMenus menus(nullptr); bind(menus);
            menus.wire();
            check(mixerMenu.findChild<QAction *>("unboundDockPanel") && !mixerMenu.isVisible(),
                "native menu docking entry exists before Cocoa displays the menu");
            prepare(mixerMenu); prepare(timelineMenu); prepare(unrelatedMenu);
            auto mixerCommand = mixerMenu.findChild<QAction *>("unboundDockPanel");
            auto timelineCommand = timelineMenu.findChild<QAction *>("unboundDockPanel");
            check(!mixerOptions.menu() && mixerCommand && mixerCommand->text() == "Dock Panel",
                "custom parented Mixer menu maps to the nearest panel inside Timeline");
            check(timelineCommand && timelineCommand->text() == "Undock Panel" && !unrelatedMenu.findChild<QAction *>("unboundDockPanel"),
                "Timeline preferences target Timeline and unrelated menus remain untouched");
            if (mixerCommand) mixerCommand->trigger();
            if (timelineCommand) timelineCommand->trigger(); QApplication::processEvents();
        }
        {
            Unbound::PanelMenus reopened(nullptr); bind(reopened); prepare(mixerMenu); prepare(mixerMenu);
            auto command = mixerMenu.findChild<QAction *>("unboundDockPanel");
            if (command) command->trigger(); QApplication::processEvents();
            check(timelineCalls == 1 && mixerCalls == 2 && toolbarCalls == 0,
                "menu callbacks rebind once after controls recreation without stale or duplicate calls");
        }
    }
    {
        QWidget nativeMain; QMenu detachedMenu; detachedMenu.setObjectName("ViewerModeMenu");
        auto command = detachedMenu.addAction("Single &Viewer Mode\tF9");
        command->setCheckable(true); command->setEnabled(false); command->setShortcut(QKeySequence(Qt::Key_F9));
        bool managed = true; int calls = 0; bool single = false;
        Unbound::ViewerModeCommand mode(nullptr, [&] { return managed; }, [&](bool value) { ++calls; single = value; });
        mode.refresh(&nativeMain); mode.refresh(&nativeMain); command->trigger(); QApplication::processEvents();
        check(calls == 1 && single && command->shortcut() == QKeySequence(Qt::Key_F9),
            "detached native viewer menu is discovered with mnemonic and shortcut suffix without remapping");
        managed = false; mode.refresh(&nativeMain);
        check(!command->isEnabled(), "leaving managed viewer mode restores the original native enabled state");
    }
    {
        QWidget nativePanel; nativePanel.setAttribute(Qt::WA_DontShowOnScreen);
        auto nativeLayout = new QHBoxLayout(&nativePanel);
        auto control = new QPushButton("Native control", &nativePanel); control->setMinimumWidth(160);
        nativeLayout->addWidget(control); nativePanel.setMinimumSize(900,100);
        const auto originalPolicy = nativePanel.sizePolicy();
        const auto originalConstraint = nativeLayout->sizeConstraint();
        Unbound::PanelWidthAllowance allowance;
        for (int cycle = 0; cycle < 20; ++cycle) {
            allowance.apply(&nativePanel); nativePanel.resize(280,200); nativeLayout->activate();
            check(nativePanel.width() == 280 && nativePanel.minimumWidth() == 240,
                "floating width allowance permits a narrow surface without outer growth");
            check(control->minimumWidth() == 160 && nativePanel.minimumHeight() == 100,
                "width allowance preserves native controls and vertical constraints");
            allowance.restore();
            check(nativePanel.minimumSize() == QSize(900,100) && nativePanel.sizePolicy() == originalPolicy &&
                nativeLayout->sizeConstraint() == originalConstraint,
                "repeated native docking restores original minimum, policy and layout constraint");
        }
        auto temporary = new QWidget; allowance.apply(temporary); delete temporary; allowance.restore();
    }
    const auto exercise = [&](QScrollArea &scroll, bool guarded) {
        scroll.setAttribute(Qt::WA_DontShowOnScreen); scroll.resize(220, 120);
        auto body = new QWidget; auto row = new QHBoxLayout(body);
        auto undock = new QPushButton("Undock", body); auto middle = new QWidget(body); middle->setMinimumWidth(700);
        auto next = new QLineEdit("Next focus target", body); row->addWidget(undock); row->addWidget(middle); row->addWidget(next);
        scroll.setWidget(body); scroll.show(); QApplication::processEvents(); QApplication::setActiveWindow(&scroll);
        undock->setFocus(); scroll.horizontalScrollBar()->setValue(0);
        if (guarded) static_cast<Unbound::ControlScroll &>(scroll).preservePosition([&] { undock->setEnabled(false); });
        else undock->setEnabled(false);
        const int result = scroll.horizontalScrollBar()->value();
        check(!(Unbound::nativeVisibilityFlags(&scroll) & 1), "scroll fixture remains absent from screen");
        scroll.hide(); return result;
    };
    QScrollArea baseline; const auto before = exercise(baseline, false);
    Unbound::ControlScroll candidate(nullptr); const auto after = exercise(candidate, true);
    check(before > 0, "reproduce Qt focus-transfer horizontal scroll jump");
    check(after == 0, "row-update guard prevents the reproduced scroll jump");
    QWidget doorRoot; doorRoot.setAttribute(Qt::WA_DontShowOnScreen); doorRoot.resize(800,400);
    auto doorLayout = new QVBoxLayout(&doorRoot);
    auto split = new QSplitter(Qt::Horizontal, &doorRoot); doorLayout->addWidget(split);
    auto neighbor = new QWidget(split); auto door = new QWidget(split);
    split->addWidget(neighbor); split->addWidget(door); doorRoot.show(); QApplication::processEvents();
    split->setSizes({500,250}); const auto openSizes = split->sizes();
    Unbound::GroupVisibility doors;
    for (int cycle = 0; cycle < 20; ++cycle) {
        doors.remember(&doorRoot); door->hide(); doorLayout->activate(); doors.restore(&doorRoot);
        const auto closedSizes = split->sizes();
        doors.remember(&doorRoot); door->show(); doorLayout->activate(); doors.restore(&doorRoot);
        check(split->sizes() == openSizes && closedSizes[1] == 0, "repeated door cycles restore exact open sizes without divider drift");
    }
    QTabWidget doorTabs; doorTabs.setAttribute(Qt::WA_DontShowOnScreen);
    auto sourceTab = new QWidget; auto otherTab = new QWidget; auto inspectorTab = new QWidget;
    doorTabs.addTab(sourceTab,"Source"); doorTabs.addTab(otherTab,"Other"); doorTabs.addTab(inspectorTab,"Inspector");
    doorTabs.setCurrentWidget(sourceTab);
    doors.tabVisibility(&doorTabs, inspectorTab, true, true);
    doors.tabVisibility(&doorTabs, inspectorTab, false, false);
    check(doorTabs.currentWidget() == sourceTab, "closing a called Inspector tab returns to the previously selected Source tab");
    doorRoot.hide();
    QMenu nativeMenu; nativeMenu.setObjectName("workspaceShowPanel"); nativeMenu.setAttribute(Qt::WA_DontShowOnScreen);
    bool calledVisible = false;
    Unbound::WorkspaceCallers callers(nullptr);
    callers.add({"testSourceCaller", "Source Monitor", [&] { return calledVisible; }, [&] { calledVisible = !calledVisible; }});
    callers.wire();
    auto caller = nativeMenu.findChild<QAction *>("testSourceCaller");
    check(caller && !caller->isChecked() && !nativeMenu.isVisible(),
        "workspace caller exists before any QWidget menu Show event");
    if (caller) caller->trigger(); QApplication::processEvents(); callers.wire();
    check(calledVisible && caller && caller->isChecked(), "native menu caller shares command visibility state");
    callers.wire(); int callerCount = 0;
    for (auto action : nativeMenu.actions()) callerCount += action->objectName() == "testSourceCaller";
    check(callerCount == 1, "repeated menu discovery does not duplicate callers");
    auto nativeInspector = nativeMenu.addAction("Inspector"); nativeInspector->setCheckable(true);
    nativeInspector->setShortcut(QKeySequence(Qt::Key_Q)); int inspectorCalls = 0;
    callers.add({"testInspectorCaller", "Inspector", [] { return true; }, [&] { ++inspectorCalls; }, [] { return true; }});
    callers.wire(); callers.wire(); nativeInspector->trigger();
    int inspectorEntries = 0;
    for (auto action : nativeMenu.actions()) inspectorEntries += action->text() == "Inspector";
    check(inspectorEntries == 1 && inspectorCalls == 1 && nativeInspector->shortcut() == QKeySequence(Qt::Key_Q),
        "existing native menu action is reused once without replacing its shortcut");
    int restoredCalls = 0;
    {
        Unbound::WorkspaceCallers oldOwner(nullptr);
        oldOwner.add({"testLifecycleCaller", "Lifecycle Panel", [] { return false; }, [] {}});
        oldOwner.wire();
    }
    {
        Unbound::WorkspaceCallers newOwner(nullptr);
        newOwner.add({"testLifecycleCaller", "Lifecycle Panel", [] { return true; }, [&] { ++restoredCalls; }});
        newOwner.wire(); newOwner.wire();
        for (auto action : nativeMenu.actions()) if (action->objectName() == "testLifecycleCaller") action->trigger();
        check(restoredCalls == 1, "caller actions rebind exactly once after controls are destroyed and recreated");
    }
    QTabWidget tabs; tabs.setAttribute(Qt::WA_DontShowOnScreen); tabs.resize(400,200);
    auto firstTab = new QWidget; auto secondTab = new QWidget;
    tabs.addTab(firstTab, "Inspector"); tabs.addTab(secondTab, "Media Pool"); tabs.show(); QApplication::processEvents();
    int tears = 0; QWidget *torn = nullptr;
    Unbound::TabTearOff tearOff(&tabs, &tabs, [&](QWidget *panel, QPoint) { ++tears; torn = panel; });
    auto bar = tabs.tabBar(); const auto tabPoint = bar->tabRect(0).center();
    const auto send = [&](QEvent::Type type, QPoint point, Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, QPointF(point), QPointF(bar->mapToGlobal(point)), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(bar, &event);
    };
    send(QEvent::MouseButtonPress, tabPoint, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, tabPoint, Qt::LeftButton, Qt::NoButton); QApplication::processEvents();
    check(tears == 0, "clicking a tab does not detach it");
    send(QEvent::MouseButtonPress, tabPoint, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, QPoint(tabPoint.x(),bar->height()+40), Qt::NoButton, Qt::LeftButton);
    QApplication::processEvents();
    check(tears == 1 && torn == firstTab, "dragging a tab outside the strip requests that exact panel");
    send(QEvent::MouseButtonRelease, tabPoint, Qt::LeftButton, Qt::NoButton);
    tabs.hide();
    QWidget nativeRoot; nativeRoot.setAttribute(Qt::WA_DontShowOnScreen);
    QWidget source(&nativeRoot); source.hide();
    QAction singleViewer(&nativeRoot); singleViewer.setText("Single Viewer Mode");
    singleViewer.setCheckable(true); singleViewer.setChecked(true);
    int commands = 0;
    QObject::connect(&singleViewer, &QAction::triggered, [&] { ++commands; });
    check(Unbound::prepareSourceViewer(&nativeRoot, &source) && commands == 1 && !singleViewer.isChecked(),
        "hidden source requests dual viewers through the existing checked native command");
    check(!Unbound::prepareSourceViewer(&nativeRoot, &source) && commands == 1,
        "source preparation does not toggle dual viewers back off");
    singleViewer.setChecked(true); singleViewer.setEnabled(false);
    check(!Unbound::prepareSourceViewer(&nativeRoot, &source) && commands == 1,
        "disabled native viewer command is left untouched");
    Unbound::MacFloatingHost host; host.setAttribute(Qt::WA_DontShowOnScreen); host.resize(400, 250);
    for (auto style : {Unbound::WindowChrome::Standard, Unbound::WindowChrome::Compact}) {
        host.setChrome(style); const auto lights = Unbound::nativeTrafficFramesForTest(&host);
        check(lights.size() == 4 && lights[3].size() == lights[2].size() && lights[3].y() == lights[2].y() &&
            lights[3].x() - lights[2].x() == lights[2].x() - lights[1].x(), "native pin matches traffic light size, spacing and baseline");
        if (style == Unbound::WindowChrome::Standard) {
            const auto title = Unbound::nativeTitleFrameForTest(&host);
            check(title.isValid() && lights.size() == 4 && title.left() > lights[3].right(),
                "native title reserves space after the fourth traffic light");
        }
        Unbound::clickNativePinForTest(&host); check(host.pinned(), "native pin light pins shared state");
        Unbound::clickNativePinForTest(&host); check(!host.pinned(), "native pin light unpins shared state");
    }
    check(Unbound::saveNativeTrafficForTest(&host, "/private/tmp/unbound-pin-candidate.png"), "render native traffic lights for inspection");
    host.setChrome(Unbound::WindowChrome::Hidden);
    check(!host.findChild<QToolButton *>("unboundPinWindow")->isHidden(), "hidden titlebar keeps a pin fallback");
    check(!(Unbound::nativeVisibilityFlags(&host) & 1), "pin fixture never appears onscreen");
    Unbound::DockCompass compass(&host); compass.setAttribute(Qt::WA_DontShowOnScreen);
    compass.display(Unbound::dockCompassZones(QRect(0,0,300,300)), QPoint(149,149));
    check(compass.grab().save("/private/tmp/unbound-compass-candidate.png"), "render five-way compass for inspection");
    check(!(Unbound::nativeVisibilityFlags(&compass) & 1), "compass fixture never appears onscreen");
    host.setChrome(Unbound::WindowChrome::Standard);
    Unbound::postNativeMoveStartForTest(&host);
    host.move(host.pos() + QPoint(30, 20));
    QElapsedTimer release; release.start();
    while (release.elapsed() < 80) { QApplication::processEvents(); QThread::msleep(1); }
    QFile events(logs.path() + "/events.jsonl"); events.open(QIODevice::ReadOnly); const auto trace = events.readAll();
    check(trace.contains("native-titlebar-release") && !QWidget::mouseGrabber(), "native titlebar notification path ends without acquiring mouse capture");
    check(!(Unbound::nativeVisibilityFlags(&host) & 1), "native move fixture remains excluded from the screen");
    check(Unbound::nativeFrontmostProcessForTest() == foreground, "background checks preserve the foreground application");
    std::printf("Scroll baseline=%d candidate=%d\n", before, after);
    if (!failures) std::puts("PASS: reproduced scrolling regression, native pins and hidden compass rendering without foreground changes");
    return failures ? 1 : 0;
}
