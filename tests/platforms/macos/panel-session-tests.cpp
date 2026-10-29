#include "native-fixture-start.h"
#include <QApplication>
#include <QCoreApplication>
#include <QStackedLayout>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QElapsedTimer>
#include <QThread>
#include <QDialog>
#include <cstdio>
#include "panelsession.h"
#include "panelcontrols.h"
#include "nativeworkspacesizing.h"
#include "controlservice.h"

// This fixture sends explicit held-button Qt moves. A native hover generated
// while the synthetic gesture yields to WindowServer has no physically held
// button and must not terminate that synthetic sequence. Real/native release
// handling remains covered separately; production input code is unchanged.
class SyntheticDragMoves : public QObject {
public:
    bool eventFilter(QObject *,QEvent *event) override {
        return event->type()==QEvent::MouseMove && event->spontaneous();
    }
};
static int failures = 0;
static void check(bool condition, const char *message)
{
    if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
static void settle()
{
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
static void settleFor(int milliseconds)
{
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < milliseconds) { settle(); QThread::msleep(2); }
}

int main(int argc, char **argv)
{
    // This suite exercises the legacy default-key contract. Do not read the
    // developer's real Resolve preset; native-preset behavior has its own suite.
    qputenv("OPENGATE_NATIVE_PRESET_SHORTCUTS","0");
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTemporaryDir logs;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", logs.path().toUtf8());
    qputenv("UNBOUND_LAUNCH_DIRECTORY", logs.path().toUtf8());
    qputenv("UNBOUND_WORKSPACE_FILE", (logs.path() + QStringLiteral("/Workspace.json")).toUtf8());
    const auto presetPath = (logs.path() + QStringLiteral("/Arrangements.json")).toUtf8();
    qputenv("UNBOUND_ARRANGEMENTS_FILE", presetPath);
    for (const auto &name : {"frameEditPageMediaPool", "frameEditPageEditIndex", "frameEditPageEffectsLibrary",
                            "frameEditPageMetadata", "frameEditAudioMeter", "frameEditPageSoundLibrary", "frameEditPageKeyframeEditor"}) {
        QWidget main; auto outer=new QVBoxLayout(&main);
        auto page=new QWidget(&main); outer->addWidget(page); auto layout=new QVBoxLayout(page);
        auto slot=new QWidget(page); slot->setObjectName(name); slot->setMinimumWidth(760);
        auto native=new QHBoxLayout(slot); auto left=new QPushButton("Left",slot); auto right=new QPushButton("Right",slot);
        native->addWidget(left); native->addStretch(); native->addWidget(right); layout->addWidget(slot);
        main.resize(1000,500); main.show(); settle();
        const auto minimum=slot->minimumSize(); const auto policy=slot->sizePolicy();
        Unbound::MacPanelSession session;
        check(session.undockContents(slot,page,name).isEmpty(),"every multipart sidebar can detach with overflow");
        session.window()->resize(320,400); settle(); settle();
        auto scroll=session.window()->findChild<QScrollArea *>();
        check(scroll && scroll->horizontalScrollBar()->maximum()>0 && session.window()->width()==320,
            "multipart sidebar viewport stays narrow without shrinking native controls");
        if (scroll) {
            scroll->horizontalScrollBar()->setValue(scroll->horizontalScrollBar()->maximum()); settle();
            check(scroll->viewport()->rect().contains(right->mapTo(scroll->viewport(),right->rect().center())),
                "last native sidebar control is reachable by scrolling");
            const auto offset=scroll->horizontalScrollBar()->value();
            page->hide(); settle(); page->show(); settle(); settle();
            check(scroll->horizontalScrollBar()->value()==offset && scroll->widget()==right->parentWidget(),
                "page return retains the same controls and overflow position");
        }
        check(session.dock().isEmpty() && slot->layout()==native && right->parentWidget()==slot &&
            slot->minimumSize()==minimum && slot->sizePolicy()==policy,
            "each multipart sidebar restores its exact native layout and constraints");
        settle();
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main);
        auto page=new QWidget(&main); outer->addWidget(page); auto layout=new QVBoxLayout(page);
        auto pool=new QWidget(page); pool->setObjectName("frameEditPageMediaPool");
        auto controls=new QHBoxLayout(pool);
        auto first=new QPushButton("First native control",pool);
        auto last=new QPushButton("Last native control",pool);
        controls->addWidget(first); controls->addStretch(); controls->addWidget(last);
        pool->setMinimumWidth(760); layout->addWidget(pool);
        main.resize(1000,500); main.show(); settle();
        const auto minimum=pool->minimumSize(); const auto policy=pool->sizePolicy();
        Unbound::MacPanelSession session;
        check(session.undock(pool,page,"Media Pool").isEmpty(),"Media Pool overflow detaches");
        session.window()->resize(320,400); settle(); settle();
        auto scroll=session.window()->findChild<QScrollArea *>("unboundPanelOverflow_frameEditPageMediaPool");
        check(scroll && scroll->horizontalScrollBar()->maximum()>0 && session.window()->width()==320,
            "narrow Media Pool has horizontal scrolling instead of compressing native controls");
        check(pool->minimumSize()==minimum && pool->sizePolicy()==policy,
            "Media Pool overflow preserves native minimum and size policy");
        if (scroll) {
            scroll->horizontalScrollBar()->setValue(scroll->horizontalScrollBar()->maximum()); settle();
            const auto target=last->mapTo(scroll->viewport(),last->rect().center());
            check(scroll->viewport()->rect().contains(target),"scrolling reaches the last native Media Pool control");
        }
        check(session.dock().isEmpty() && pool->parentWidget()==page && pool->minimumSize()==minimum && pool->sizePolicy()==policy,
            "Media Pool docking restores native ownership and constraints");
        settle();
    }
    {
        QWidget slot; slot.setMinimumSize(700,400); slot.setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
        const auto minimum=slot.minimumSize(); const auto policy=slot.sizePolicy(); const bool hidden=slot.isHidden();
        Unbound::VacatedContents vacancy; vacancy.hide(&slot);
        check(slot.isHidden() && slot.minimumSize()==QSize(0,0) && slot.sizePolicy().horizontalPolicy()==QSizePolicy::Ignored,
            "vacated native container releases its footprint and preferred width");
        slot.show(); settle();
        check(slot.isHidden(),"native visibility refresh cannot reopen a leased empty content slot");
        vacancy.restore();
        check(slot.minimumSize()==minimum && slot.sizePolicy()==policy && slot.isHidden()==hidden,
            "native content-container lease restores exact dimensions, policy and hidden state");
        slot.show(); settle(); check(!slot.isHidden(),"released slot accepts native Show again after docking");
    }
    {
        QWidget main; main.setMinimumSize(1000,700); main.resize(1200,800);
        auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto contents=new QHBoxLayout(page); auto remaining=new QWidget(page); remaining->setMinimumSize(200,100); contents->addWidget(remaining);
        main.show(); settle(); const auto baseline=main.geometry(); const auto minimum=main.minimumSize(); const auto constraint=outer->sizeConstraint();
        Unbound::NativeWorkspaceSizing sizing(nullptr); bool expanding=true;
        sizing.blocked=[&] { return expanding; }; sizing.setPanels(&main,page,{"detached"}); settle();
        check(main.geometry()==baseline && main.minimumSize()==minimum,"temporary maximize does not resize the native workspace behind it");
        expanding=false; sizing.setPanels(&main,page,{"detached"}); settle();
        check(main.minimumWidth()<minimum.width() && main.geometry()==baseline,"detachment releases the resize floor without shrinking the user workspace");
        check(remaining->minimumSize()==QSize(200,100),"native outer sizing never relaxes remaining content constraints");
        sizing.restore(); settle();
        check(main.minimumSize()==minimum && outer->sizeConstraint()==constraint && main.geometry()==baseline,"native sizing restores exact captured constraints and automatic geometry");
        sizing.setPanels(&main,page,{"detached"}); settle(); main.move(main.pos()+QPoint(20,15)); const auto moved=main.pos();
        sizing.restore(); settle(); check(main.pos()==moved,"native restore preserves deliberate user movement after automatic sizing");
        sizing.prepare(&main,page); main.setMinimumSize(700,500);
        sizing.setPanels(&main,page,{"detached"}); settle(); sizing.restore(); settle();
        check(main.minimumSize()==minimum,"baseline is captured before native detachment changes its own minimum");
    }
    {
        QWidget root; QObject owner; bool ready=false; int completed=0;
        new Unbound::SurfaceReadiness(&owner,&root,[] { return true; },[&] { return ready; },
            [&](bool success) { check(success,"surface readiness completes from actual readiness"); ++completed; });
        settle(); check(completed==0,"surface preparation does not finish before it is ready");
        ready=true; QEvent layout(QEvent::LayoutRequest); QApplication::sendEvent(&root,&layout); settle();
        check(completed==1,"layout readiness completes without a fixed preparation sleep");
        QApplication::sendEvent(&root,&layout); settle();
        check(completed==1,"completed readiness cannot dispatch a second undock");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); auto outer = new QVBoxLayout(&main);
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page);
        auto row = new QVBoxLayout(page); auto stack = new QStackedWidget(page);
        stack->setObjectName("stackedEditSourceOfflinePlayers"); stack->addWidget(new QLabel("No clip loaded",stack)); row->addWidget(stack);
        main.show(); settle();
        auto surface = Unbound::panelSurface(page,"widgetEditSourcePlayer");
        check(surface == stack,"empty Source resolves to its native offline stack before a player or clip exists");
        Unbound::MacPanelSession session;
        check(session.undock(surface,page,"Source Monitor",true).isEmpty(),"offline Source can detach without loading media");
        check(session.dock().isEmpty() && stack->parentWidget() == page,"offline Source returns to its original native slot");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow"); auto outer = new QVBoxLayout(&main);
        auto page = new QWidget(&main); outer->addWidget(page); auto row = new QHBoxLayout(page);
        auto leaving = new QWidget(page); leaving->setMinimumSize(900,300); row->addWidget(leaving);
        auto remaining = new QWidget(page); remaining->setMinimumSize(200,100); row->addWidget(remaining);
        main.show(); settle(); const auto before = main.minimumSizeHint();
        Unbound::MacPanelSession session;
        check(session.undock(leaving,page,"Vacancy fixture").isEmpty(),"native panel detaches for vacancy check"); settle();
        auto slot = page->findChild<QWidget *>("unboundVacatedSlot_");
        check(slot && slot->isHidden() && slot->minimumSize() == QSize(0,0),"vacated native slot does not retain detached minimum dimensions");
        check(main.minimumSizeHint().width() < before.width(),"native layout requirement decreases while remaining native content is retained");
        check(session.dock().isEmpty() && leaving->minimumSize() == QSize(900,300),"redocking preserves the native panel's original constraints");
    }
    {
        QWidget main; auto layout=new QVBoxLayout(&main); auto panel=new QWidget(&main); layout->addWidget(panel);
        auto content=new QVBoxLayout(panel); auto button=new QPushButton("Native control",panel); button->setFixedSize(100,40); content->addWidget(button);
        main.show(); settle(); const auto font=button->font();
        Unbound::MacPanelSession session; session.setInterfaceScale(0.8);
        check(session.undock(panel,&main,"Scaled panel").isEmpty(),"scaled native widget detaches"); settle();
        check(button->minimumSize()==QSize(80,32),"saved panel scale applies on detachment");
        QWidget group(&main,Qt::Tool); group.setObjectName("unboundFloating_scaleGroup");
        check(session.attachToGroup(&group).isEmpty(),"scaled widget joins group"); settle();
        check(button->minimumSize()==QSize(80,32),"group transfer does not compound panel scale");
        session.detachFromGroup(); settle();
        check(button->minimumSize()==QSize(80,32),"scaled panel detaches from group without compounding");
        check(session.dock().isEmpty(),"scaled panel returns to native workspace"); settle();
        check(button->minimumSize()==QSize(100,40) && button->font()==font && qAbs(session.interfaceScale()-0.8)<0.001,
            "native docking restores original metrics while retaining independent next-detach preference");
    }
    {
        QWidget main; auto layout=new QVBoxLayout(&main); auto slot=new QWidget(&main); layout->addWidget(slot);
        auto content=new QVBoxLayout(slot); auto button=new QPushButton("Native transferred control",slot); button->setFixedSize(120,50); content->addWidget(button);
        main.show(); settle(); const auto font=button->font();
        Unbound::MacPanelSession session; session.setInterfaceScale(1.2);
        check(session.undockContents(slot,&main,"Scaled contents").isEmpty(),"content-layout panel detaches with independent scale"); settle();
        check(button->minimumSize()==QSize(144,60),"transferred layout receives native metric scale");
        QWidget group(&main,Qt::Tool); group.setObjectName("unboundFloating_scaleContentsGroup");
        check(session.attachToGroup(&group).isEmpty(),"scaled content layout transfers into group"); settle();
        check(button->minimumSize()==QSize(144,60),"content-layout group keeps independent scale");
        session.detachFromGroup();
        check(session.dock().isEmpty(),"scaled content layout restores original slot"); settle();
        check(slot->layout()==content && button->minimumSize()==QSize(120,50) && button->font()==font,
            "content-layout restoration returns exact native control metrics and ownership");
    }
    {
        Unbound::ArrangementStore store;
        QString first, copy;
        const QJsonObject workspace{{"panels", QJsonObject{{"Timeline", QJsonObject{{"floating", true}, {"x", -1200}}}}}};
        check(store.create("Editing", workspace, first).isEmpty(), "create named arrangement");
        check(!store.create(" editing ", workspace, copy).isEmpty(), "reject duplicate names without changing data");
        check(store.create("Editing copy", store.find(first)["workspace"].toObject(), copy).isEmpty(), "duplicate arrangement with independent identity");
        check(store.rename(first, "Review").isEmpty(), "rename keeps arrangement identity");
        check(store.update(first, QJsonObject{{"panels", QJsonObject()}}).isEmpty(), "explicit update");
        Unbound::ArrangementStore reloaded;
        check(reloaded.find(first)["name"] == "Review" && reloaded.find(copy)["workspace"].toObject() == workspace,
            "arrangements persist and duplicate remains independent");
        check(store.remove(first).isEmpty() && store.remove(copy).isEmpty(), "delete presets without current workspace mutation");
        qputenv("UNBOUND_ARRANGEMENTS_FILE", (logs.path() + "/Invalid.json").toUtf8());
        QFile invalid(logs.path() + "/Invalid.json"); invalid.open(QIODevice::WriteOnly); invalid.write("broken"); invalid.close();
        Unbound::ArrangementStore damaged;
        check(!damaged.error().isEmpty() && !damaged.create("New", workspace, first).isEmpty(), "invalid files cannot be silently overwritten");
        invalid.open(QIODevice::ReadOnly); check(invalid.readAll() == "broken", "invalid preset data preserved for recovery");
        qputenv("UNBOUND_ARRANGEMENTS_FILE", presetPath);
    }
    {
        QWidget main; auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); outer->addWidget(page);
        auto row=new QVBoxLayout(page); auto panel=new QWidget(page); row->addWidget(panel); main.show(); settle();
        Unbound::MacPanelSession session;
        check(session.placement()==Unbound::PanelPlacement::NativeSlot,"unleased panel starts in its original native slot");
        check(session.undock(panel,page,"Placement fixture").isEmpty() && session.floating(),"leased standalone content is floating");
        QWidget surface(&main,Qt::Tool); auto cell=new QWidget(&surface);
        check(session.attachToGroup(cell).isEmpty(),"placement fixture joins a managed surface");
        surface.setWindowFlags(Qt::Widget);
        check(session.active() && !session.floating() && session.placement()==Unbound::PanelPlacement::NativeSurface,
            "content leased into a native surface is managed but not floating");
        session.detachFromGroup();
        check(session.floating(),"detaching an embedded member returns floating placement");
        check(session.dock().isEmpty() && session.placement()==Unbound::PanelPlacement::NativeSlot,"native restoration ends the original content lease");
    }
    Unbound::WindowDiagnostics windowLogs(&app);
    app.installEventFilter(&windowLogs);
    app.setQuitOnLastWindowClosed(false);
    {
        const QRect peer(100, 100, 200, 200);
        check(Unbound::snapWindowFrame(QRect(305, 105, 200, 200), {peer}, {}) == QRect(300, 100, 200, 200),
            "snapping aligns adjacent frames and equal top edges");
        check(Unbound::snapWindowFrame(QRect(305, 1000, 200, 200), {peer}, {}) == QRect(305, 1000, 200, 200),
            "snapping ignores panels far away on the other axis");
        check(Unbound::snapWindowFrame(QRect(-1914, 8, 400, 300), {}, {QRect(-1920, 0, 1920, 1080)}) == QRect(-1920, 0, 400, 300),
            "snapping supports display edges with negative coordinates");
        check(Unbound::snapWindowFrame(QRect(313, 100, 200, 200), {peer}, {}).x() == 313,
            "snapping does not pull windows outside the threshold");
        QList<QLine> guides;
        const auto aligned = Unbound::alignWindowEdges(QRect(106, 1000, 200, 200), {peer}, guides);
        check(aligned.x() == 100 && aligned.y() == 1000 && !guides.isEmpty() && std::abs(guides.first().dy()) > 1000,
            "virtual edge guides align separated windows without closing the gap");
        check(Unbound::localDockZone(peer, peer.center()).edge == Qt::Edge(0) && Unbound::localDockZone(peer, peer.center()).active,
            "only a deliberate center target proposes a tab");
        check(!Unbound::localDockZone(peer, QPoint(peer.left() + 35, peer.top() + 35)).target.isValid(),
            "ordinary panel content is not a docking surface");
        check(!Unbound::localDockZone(peer, QPoint(peer.right(), peer.top() + 25)).target.isValid(),
            "edge targets do not chase the pointer along an entire window");
        const QRect partiallyOutside(-400, 200, 600, 500), display(0, 0, 1920, 1080);
        check(Unbound::keepGripReachable(partiallyOutside, {display}) == partiallyOutside,
            "partly off-screen windows keep their intentional position and size");
        auto rescued = Unbound::keepGripReachable(QRect(-2000, -2000, 600, 500), {display});
        check(rescued.size() == QSize(600, 500) && rescued.top() >= 0 && rescued.right() >= 95,
            "fully lost windows regain a reachable grip without shrinking");
        auto local = Unbound::localDockZone(peer, QPoint(peer.right(), peer.center().y()));
        check(local.active && local.target.size() == QSize(18, 18) && local.edge == Qt::RightEdge && local.preview.width() == peer.width() / 2,
            "local target previews only the intended half split");
        check(!Unbound::localDockZone(peer, QPoint(peer.right() + 25, peer.center().y())).target.isValid(),
            "docking indicators stay within a narrow edge approach");
        const auto nearZone = Unbound::localDockZone(peer, QPoint(peer.right() + 12, peer.center().y()));
        check(nearZone.target.isValid() && !nearZone.active, "approaching an edge does not activate the visible target");
        Unbound::MacFloatingHost target, moving;
        target.setObjectName("unboundFloating_snapTarget"); moving.setObjectName("unboundFloating_snapMoving");
        target.setGeometry(150, 260, 240, 180); moving.setGeometry(600, 260, 240, 180);
        moving.setChrome(Unbound::WindowChrome::Standard);
        target.show(); moving.show(); settle();
        const auto lights = Unbound::nativeTrafficFramesForTest(&moving);
        check(lights.size() == 4 && lights[3].size() == lights[2].size() && lights[3].y() == lights[2].y() &&
            lights[3].x() - lights[2].x() == lights[2].x() - lights[1].x(), "native pin matches traffic-light size, alignment and spacing");
        Unbound::clickNativePinForTest(&moving); check(moving.pinned(), "native pin light controls shared pin state");
        Unbound::clickNativePinForTest(&moving); check(!moving.pinned(), "native pin light unpins without replacing window");
        moving.setChrome(Unbound::WindowChrome::Hidden);
        check(moving.findChild<QToolButton *>("unboundPinWindow")->isVisible(), "hidden titlebar retains a reachable circular pin fallback");
        moving.setChrome(Unbound::WindowChrome::Standard);
        int nativeReleases = 0;
        auto watch = Unbound::watchNativePointerRelease(&moving, [&](QPoint position, bool option) { check(!option, "native release preserves Option state"); Q_UNUSED(position); ++nativeReleases; });
        Unbound::postNativePointerReleaseForTest(&moving); settleFor(80);
        check(nativeReleases == 1, "native Cocoa mouse-up reaches the GUI-thread release fallback");
        delete watch;
        Unbound::postNativePointerReleaseForTest(&moving); settleFor(50);
        check(nativeReleases == 1, "completed drag removes its native event observer");
        const auto drag = [&](Qt::KeyboardModifiers modifiers) {
            const QPoint press = moving.mapToGlobal(QPoint(100, 7));
            const QPoint desiredFrame(target.frameGeometry().right() + 6, target.frameGeometry().top());
            const QPoint delta = desiredFrame - moving.frameGeometry().topLeft();
            QMouseEvent down(QEvent::MouseButtonPress, QPointF(100, 7), QPointF(press), Qt::LeftButton, Qt::LeftButton, modifiers);
            QApplication::sendEvent(&moving, &down);
            // Native title-bar callbacks enter here; standard chrome has no
            // second Qt grip row for a synthetic client-area press to hit.
            moving.beginPointerDrag(press);
            QMouseEvent move(QEvent::MouseMove, QPointF(100, 7) + delta, QPointF(press + delta), Qt::NoButton, Qt::LeftButton, modifiers);
            QApplication::sendEvent(&moving, &move);
            auto preview = moving.findChild<QWidget *>("unboundSnapPreview");
            check(!preview || !preview->isVisible(), "free movement never paints a docking rectangle");
            QMouseEvent up(QEvent::MouseButtonRelease, QPointF(100, 7), QPointF(press + delta), Qt::LeftButton, Qt::NoButton, modifiers);
            QApplication::sendEvent(&moving, &up); settle();
        };
        drag(Qt::NoModifier);
        check(moving.frameGeometry().left() == target.frameGeometry().right() + 6, "free release does not force attachment to a nearby edge");
        drag(Qt::AltModifier);
        check(moving.frameGeometry().left() == target.frameGeometry().right() + 6, "Option release preserves unsnapped placement");
        QWidget unrelated;
        const auto begin = [&] {
            const auto point = moving.mapToGlobal(QPoint(100, 7));
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 7), QPointF(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(&moving, &press); moving.beginPointerDrag(point); return point;
        };
        auto point = begin();
        QMouseEvent otherRelease(QEvent::MouseButtonRelease, QPointF(), QPointF(point + QPoint(30, 15)), Qt::LeftButton, Qt::NoButton, Qt::AltModifier);
        QApplication::sendEvent(&unrelated, &otherRelease);
        const auto releasedGeometry = moving.geometry();
        QMouseEvent staleMove(QEvent::MouseMove, QPointF(), QPointF(point + QPoint(80, 50)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&moving, &staleMove);
        check(QWidget::mouseGrabber() != &moving && moving.geometry() == releasedGeometry, "left release through another window ends drag without a right click");
        point = begin();
        QMouseEvent missedRelease(QEvent::MouseMove, QPointF(), QPointF(point), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&unrelated, &missedRelease);
        check(QWidget::mouseGrabber() != &moving, "missing mouse-up is recovered when left button is no longer held");
        begin(); QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&unrelated, &escape);
        check(QWidget::mouseGrabber() != &moving, "Escape cancels capture from any receiver");
        // Reproduce a native grab surviving cancellation during Cocoa activation.
        moving.grabMouse();
        QApplication::sendEvent(&unrelated, &escape);
        check(QWidget::mouseGrabber() != &moving, "orphaned native capture is released even after drag state was cleared");
        auto orphanPoint = moving.mapToGlobal(QPoint(100, 70));
        moving.grabMouse();
        QMouseEvent orphanClick(QEvent::MouseButtonPress, QPointF(100, 70), QPointF(orphanPoint), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&moving, &orphanClick);
        check(QWidget::mouseGrabber() != &moving, "content click cannot preserve an orphaned mouse grab");
    }
    {
        QWidget page;
        auto layout = new QHBoxLayout(&page);
        auto left = new QWidget(&page);
        auto right = new QWidget(&page);
        layout->addWidget(left, 2, Qt::AlignTop);
        layout->addWidget(right, 3);
        page.show(); settle();
        QPointer<QWidget> surviving = left;
        Unbound::MacPanelSession session;
        for (int i = 0; i < 30; ++i) {
            check(session.undock(left, &page, "Panel").isEmpty(), "box detach");
            check(left->parentWidget() == session.window(), "content moved into host");
            check(layout->count() == 2 && layout->itemAt(1)->widget() == right, "sibling preserved");
            check(!session.undock(left, &page, "Panel").isEmpty(), "double detach rejected");
            check(session.dock().isEmpty(), "box restore");
            check(left->parentWidget() == &page && layout->itemAt(0)->widget() == left, "original parent and position");
            check(layout->stretch(0) == 2 && layout->stretch(1) == 3 &&
                  layout->itemAt(0)->alignment() == Qt::AlignTop, "stretch and alignment preserved");
            settle();
            check(surviving && left->isVisible(), "content survives host deletion");
        }
        check(session.undock(left, &page, "Panel").isEmpty(), "close setup");
        auto retained=session.window();const auto retainedFrame=retained->geometry();
        check(retained->close() && session.active() && session.hiddenByUser() && !retained->isVisible(), "close hides retained panel");
        session.setHiddenByUser(false);
        check(session.window()==retained && retained->geometry()==retainedFrame, "recall preserves closed panel host and frame");
        check(session.dock().isEmpty() && left->parentWidget()==&page, "explicit dock restores original owner after close and recall");
        settle();
        left->hide();
        check(!session.undock(left, &page, "Panel").isEmpty() && left->parentWidget() == &page, "hidden panel rejected unchanged");
    }
    {
        QWidget page;
        auto layout = new QGridLayout(&page);
        auto content = new QWidget(&page);
        layout->addWidget(content, 1, 2, 2, 3, Qt::AlignLeft);
        page.show(); settle();
        Unbound::MacPanelSession session;
        check(session.undock(content, &page, "Grid").isEmpty(), "grid detach");
        check(session.dock().isEmpty(), "grid restore");
        int row, column, rows, columns;
        layout->getItemPosition(layout->indexOf(content), &row, &column, &rows, &columns);
        check(row == 1 && column == 2 && rows == 2 && columns == 3 &&
              layout->itemAt(0)->alignment() == Qt::AlignLeft, "grid span and alignment preserved");
    }
    {
        QWidget page;
        auto box = new QVBoxLayout(&page);
        auto splitter = new QSplitter(&page);
        auto a = new QWidget(splitter);
        auto b = new QWidget(splitter);
        splitter->addWidget(a); splitter->addWidget(b);
        box->addWidget(splitter);
        page.resize(800, 400); page.show(); settle();
        splitter->setSizes({220, 560});
        const auto sizes = splitter->sizes();
        Unbound::MacPanelSession session;
        check(session.undock(a, &page, "Split").isEmpty(), "splitter detach");
        check(session.dock().isEmpty() && splitter->widget(0) == a && splitter->widget(1) == b, "splitter order restored");
        check(splitter->sizes() == sizes, "splitter sizes restored");
    }
    {
        QWidget page;
        page.resize(900, 600);
        QWidget source(&page), program(&page), timeline(&page);
        source.setGeometry(0, 0, 400, 250);
        program.setGeometry(420, 0, 400, 250);
        timeline.setGeometry(0, 270, 820, 300);
        page.show(); settle();
        const QRect sourceGeometry = source.geometry(), programGeometry = program.geometry(), timelineGeometry = timeline.geometry();
        Unbound::MacPanelSession sourceSession, programSession, timelineSession;
        check(sourceSession.undock(&source, &page, "Source").isEmpty(), "independent source detach");
        check(programSession.undock(&program, &page, "Program").isEmpty(), "independent program detach");
        check(timelineSession.undock(&timeline, &page, "Timeline").isEmpty(), "independent timeline detach");
        page.hide(); settle();
        check(!sourceSession.window()->isVisible() && !programSession.window()->isVisible() &&
              !timelineSession.window()->isVisible(), "page change hides all hosts");
        page.show(); settle();
        check(sourceSession.window()->isVisible() && programSession.window()->isVisible() &&
              timelineSession.window()->isVisible(), "page return shows all hosts");
        check(programSession.dock().isEmpty() && sourceSession.active() && timelineSession.active(), "program docks independently");
        check(sourceSession.dock().isEmpty() && timelineSession.dock().isEmpty(), "remaining panels dock");
        check(source.geometry() == sourceGeometry && program.geometry() == programGeometry &&
              timeline.geometry() == timelineGeometry, "manual geometry restored");
    }
    {
        QWidget page;
        auto row = new QHBoxLayout(&page);
        auto stack = new QStackedWidget(&page);
        auto source = new QWidget;
        auto offline = new QWidget;
        auto sourceTimeline = new QWidget;
        stack->addWidget(source); stack->addWidget(offline); stack->addWidget(sourceTimeline);
        auto program = new QWidget(&page);
        row->addWidget(stack); row->addWidget(program);
        page.show(); settle();
        Unbound::MacPanelSession session;
        check(session.undock(source, &page, "Source").isEmpty(), "source child detaches from stack");
        check(source->parentWidget() == session.window() && stack->parentWidget() == &page &&
              offline->parentWidget() == stack && sourceTimeline->parentWidget() == stack &&
              program->parentWidget() == &page, "source floats without stack, alternate viewers or program");
        check(session.dock().isEmpty() && stack->widget(0) == source && stack->widget(1) == offline &&
              stack->widget(2) == sourceTimeline && stack->currentWidget() == source, "source stack order and selection restored");
        check(session.undock(source, &page, "Source").isEmpty(), "second source detach");
        stack->setCurrentWidget(offline);
        check(session.dock().isEmpty() && stack->currentWidget() == offline && source->isHidden(), "native viewer selection change survives docking");
    }
    {
        QWidget main;
        main.setObjectName(QStringLiteral("UiMainWindow"));
        auto page = new QWidget(&main);
        page->setObjectName(QStringLiteral("WStackPage_Conform"));
        auto mainLayout = new QVBoxLayout(&main);
        mainLayout->addWidget(page);
        auto row = new QHBoxLayout(page);
        auto stack = new QStackedWidget(page);
        auto source = new QWidget;
        source->setObjectName(QStringLiteral("widgetEditSourcePlayer"));
        auto sourceOptions = new QToolButton(source);
        sourceOptions->setObjectName(QStringLiteral("tbtnPlayerOptions"));
        auto sourceMenu = new QMenu(sourceOptions);
        sourceOptions->setMenu(sourceMenu);
        auto nativeOption = sourceMenu->addAction(QStringLiteral("Native viewer setting"));
        stack->addWidget(source); row->addWidget(stack);
        auto program = new QWidget(page);
        program->setObjectName(QStringLiteral("widgetEditTimelinePlayer")); row->addWidget(program);
        main.show(); settle();
        Unbound::ArrangementStore presetStore;
        QString presetId, groupedId;
        const QJsonObject sourceArrangement{{"panels", QJsonObject{{"widgetEditSourcePlayer", QJsonObject{
            {"floating", true}, {"hidden", false}, {"x", 80}, {"y", 90}, {"width", 480}, {"height", 320}}}}}};
        check(presetStore.create("Source layout", sourceArrangement, presetId).isEmpty(), "prepare restore fixture");
        {
        prepareNativeFixture();PanelControls controls;
        controls.show(); settle();
        controls.resize(300, 240); settle();
        auto optionsScroll = controls.findChild<QScrollArea *>(QStringLiteral("panelOptionsScroll"));
        check(optionsScroll && controls.height() == 240 && controls.width() == 300, "controls can shrink around a scrollable panel list");
        check(controls.close() && !controls.isVisible(), "closing controls hides them without vetoing native application shutdown");
        controls.show(); settle();
        check(controls.findChild<QPushButton *>("bulk_1")->isVisibleTo(&controls), "Dock All remains outside scrolling content");
        auto sourceUndock = controls.findChild<QPushButton *>(QStringLiteral("undock_widgetEditSourcePlayer"));
        auto sourceDock = controls.findChild<QPushButton *>(QStringLiteral("dock_widgetEditSourcePlayer"));
        auto programDock = controls.findChild<QPushButton *>(QStringLiteral("dock_widgetEditTimelinePlayer"));
        check(sourceUndock && sourceDock && programDock, "controls have distinct panel identities");
        check(!sourceDock->isEnabled() && !programDock->isEnabled(), "inactive Dock buttons are disabled");
        optionsScroll->ensureWidgetVisible(sourceUndock);
        sourceUndock->setFocus(); settle();
        const QPoint scrollBefore(optionsScroll->horizontalScrollBar()->value(), optionsScroll->verticalScrollBar()->value());
        sourceUndock->click(); settle();
        check(QPoint(optionsScroll->horizontalScrollBar()->value(), optionsScroll->verticalScrollBar()->value()) == scrollBefore,
            "Undock row updates preserve the controls scroll position in a small window");
        check(sourceDock->isEnabled() && !sourceUndock->isEnabled() && !programDock->isEnabled(), "only detached panel enables Dock");
        check(!(Unbound::nativeVisibilityFlags(source->window()) & 2), "native tool panels do not auto-hide on application deactivation");
        auto hideBackground = controls.findChild<QCheckBox *>("hideResolveBackground");
        controls.activateWindow(); settle(); const auto backgroundKey = Unbound::nativeKeyWindowNumber();
        hideBackground->setChecked(true); settle();
        check(!(Unbound::nativeVisibilityFlags(&main) & 1) && (Unbound::nativeVisibilityFlags(source->window()) & 1) && main.isVisible(),
            "background hides natively while detached panel and Qt page ownership remain alive");
        check(Unbound::nativeKeyWindowNumber() == backgroundKey, "background hiding preserves the active controls key window");
        hideBackground->setChecked(false); settle(); check(Unbound::nativeVisibilityFlags(&main) & 1, "background can be explicitly recalled");
        source->window()->move(20000, -20000);
        controls.findChild<QPushButton *>("hide_widgetEditSourcePlayer")->click();
        controls.findChild<QPushButton *>("center_widgetEditSourcePlayer")->click(); settle();
        auto rescueScreen = QGuiApplication::screenAt(QCursor::pos()); if (!rescueScreen) rescueScreen = QGuiApplication::primaryScreen();
        check(source->window()->isVisible() && rescueScreen->availableGeometry().contains(source->window()->frameGeometry()),
            "Center recalls a hidden off-screen panel into the current viewport");
        programDock->click();
        check(source->parentWidget() != stack, "unrelated Dock cannot claim or change source state");
        sourceDock->click(); settle();
        check(source->parentWidget() == stack && !sourceDock->isEnabled() && sourceUndock->isEnabled(), "source Dock button restores correct child");
        sourceUndock->click(); settle();
        auto retainedSource=source->window();
        check(retainedSource->close(), "source Close is accepted"); settle();
        check(source->window()==retainedSource && !retainedSource->isVisible() && sourceDock->isEnabled(), "closing host retains placement and explicit Dock availability");
        auto presetSelector = controls.findChild<QComboBox *>("arrangements");
        presetSelector->setCurrentIndex(presetSelector->findData(presetId));
        controls.findChild<QPushButton *>("restoreArrangement")->click(); settle();
        check(source->window() != &main && sourceDock->isEnabled() && !programDock->isEnabled(), "preset restores only requested panel");
        check(source->window()->size() == QSize(480, 320), "preset restores independent window dimensions");
        sourceDock->click(); settle();
        check(controls.findChild<QLabel *>("arrangementState")->text().startsWith("Modified"), "workspace changes show unsaved preset state");
        Unbound::ArrangementStore unchanged;
        check(unchanged.find(presetId)["workspace"].toObject() == sourceArrangement, "workspace autosave never overwrites named preset");
        check(presetStore.remove(presetId).isEmpty(), "fixture cleans saved preset");
        sourceMenu->show(); settle();
        auto dockCommand = sourceMenu->findChild<QAction *>("unboundDockPanel");
        check(dockCommand && dockCommand->text() == "Undock Panel", "native menu reflects docked panel state");
        check(sourceMenu->actions().contains(nativeOption), "native viewer menu items preserved");
        sourceMenu->hide(); dockCommand->trigger(); settle(); settle();
        check(sourceDock->isEnabled() && !sourceUndock->isEnabled(), "native menu uses the same undock operation and controls state");
        sourceMenu->show(); settle();
        check(dockCommand->text() == "Dock Panel" && sourceMenu->findChildren<QAction *>("unboundDockPanel").size() == 1,
            "native menu refreshes shared state without duplicate commands");
        sourceMenu->hide(); dockCommand->trigger(); settle(); settle();
        check(source->parentWidget() == stack && !sourceDock->isEnabled(), "native menu redocks original panel and updates controls");
        auto sharedOptions = new QToolButton(program);
        sharedOptions->setObjectName(QStringLiteral("tbtnPlayerOptions")); sharedOptions->setMenu(sourceMenu);
        sourceMenu->show(); settle();
        check(!dockCommand->isVisible(), "ambiguous shared native menus cannot target the wrong panel");
        sourceMenu->hide(); sharedOptions->setMenu(nullptr);
        sourceMenu->show(); settle(); check(dockCommand->isVisible(), "identified menu restores its command after ambiguity clears"); sourceMenu->hide();
        auto expandSource = controls.findChild<QPushButton *>("expand_widgetEditSourcePlayer");
        expandSource->click(); settleFor(160);
        check(sourceDock->isEnabled() && expandSource->text() == "Restore Size", "expand temporarily detaches docked panel");
        Unbound::WorkspaceSettings duringExpanded;
        check(!duringExpanded.data["panels"].toObject()["widgetEditSourcePlayer"].toObject()["floating"].toBool(), "temporary expansion does not persist as undocked");
        expandSource->click(); settle();
        check(source->parentWidget() == stack && !sourceDock->isEnabled(), "restore expansion returns docked panel to original slot");
        sourceUndock->click(); settle();
        const QRect beforeExpand = source->window()->geometry();
        expandSource->click(); settleFor(160);
        expandSource->click(); settle();
        check(source->window()->geometry() == beforeExpand && sourceDock->isEnabled(), "floating expansion restores exact original geometry");
        sourceDock->click(); settle();
        expandSource->click(); settleFor(160);
        source->window()->close(); settle();
        check(source->parentWidget() == stack && expandSource->text() == "Expand", "closing expanded panel clears temporary state and restores docking");
        QMenu unrelated(&main); unrelated.addAction("Unrelated"); unrelated.show(); settle();
        check(!unrelated.findChild<QAction *>("unboundDockPanel"), "unrelated menus are not modified");
        unrelated.hide();
        controls.findChild<QPushButton *>("group_0")->click(); settle(); settle(); settle();
        check(source->window()==program->window(),"ready panel grouping completes without a fixed preparation sleep");
        settleFor(350);
        check(source->window() == program->window() && source->window()->objectName().startsWith("unboundFloating_group_"),
            "controls join Source and Program into a single host");
        check(expandSource->isEnabled(), "group members allow reversible individual expansion");
        auto groupedHost = source->window(); const auto groupedGeometry = groupedHost->geometry();
        expandSource->click(); settleFor(160);
        check(source->window() != groupedHost && expandSource->text() == "Restore Size", "grouped source borrows a temporary native host");
        expandSource->click(); settleFor(200);
        check(source->window() == groupedHost && groupedHost->geometry() == groupedGeometry, "grouped source expansion returns to its original group geometry");
        const auto priorPointer = QCursor::pos();
        QCursor::setPos(source->mapToGlobal(QPoint(source->width() / 2, source->height() / 2)));
        QKeyEvent maximizeOverride(QEvent::ShortcutOverride, Qt::Key_QuoteLeft, Qt::NoModifier);
        QApplication::sendEvent(source, &maximizeOverride);
        check(maximizeOverride.isAccepted(), "hovered panel reserves backtick before competing native shortcuts");
        QKeyEvent maximizeKey(QEvent::KeyPress, Qt::Key_QuoteLeft, Qt::NoModifier);
        QApplication::sendEvent(source, &maximizeKey); settleFor(160);
        check(expandSource->text() == "Restore Size" && source->window()->property("unboundExpanded").toBool(), "backtick expands hovered grouped panel with temporary foreground priority");
        QCursor::setPos(controls.mapToGlobal(QPoint(10, 10)));
        QApplication::sendEvent(&controls, &maximizeKey); settleFor(180);
        check(source->window() == groupedHost && expandSource->text() == "Expand", "second backtick restores same panel even after pointer leaves");
        QCursor::setPos(priorPointer);
        controls.findChild<QPushButton *>("group_2")->click(); settle();
        auto draggedSource = source->window();
        auto targetProgram = program->window();
        controls.move(1100, 700);
        draggedSource->setGeometry(40, 300, 320, 240); targetProgram->setGeometry(420, 300, 320, 240);
        targetProgram->raise(); draggedSource->raise(); settle();
        // A Qt event drain does not synchronize WindowServer's on-screen list.
        // Establish that precondition explicitly before simulating a drag.
        QElapsedTimer presentation; presentation.start();
        bool presented = false;
        do {
            const auto ordered = Unbound::nativeOrderedWindowNumbers();
            presented = ordered.contains(Unbound::nativeWindowNumber(targetProgram)) &&
                ordered.contains(Unbound::nativeWindowNumber(draggedSource));
            if (!presented) { settle(); QThread::msleep(2); }
        } while (!presented && presentation.elapsed() < 500);
        check(presented, "native drag fixture windows reach the on-screen stack before pointer input");
        SyntheticDragMoves syntheticMoves;qApp->installEventFilter(&syntheticMoves);
        const QPoint press = draggedSource->mapToGlobal(QPoint(100, 7));
        const QPoint dockPointer(targetProgram->frameGeometry().right(), targetProgram->frameGeometry().center().y());
        const QPoint delta = dockPointer - press;
        QMouseEvent down(QEvent::MouseButtonPress, QPointF(100, 7), QPointF(press), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(draggedSource, &down);
        static_cast<Unbound::MacFloatingHost *>(draggedSource)->beginPointerDrag(press + QPoint(500,500));
        QMouseEvent move(QEvent::MouseMove, QPointF(100, 7) + delta, QPointF(press + delta), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(draggedSource, &move);
        auto attachmentPreview = draggedSource->findChild<QWidget *>("unboundSnapPreview");
        auto dockTarget = draggedSource->findChild<QWidget *>("unboundDockTarget");
        if (!attachmentPreview || !attachmentPreview->isVisible()) {
            std::fprintf(stderr, "Preview diagnostic pointer=%d,%d source=%lld target=%lld targetVisible=%d\n", dockPointer.x(), dockPointer.y(),
                (long long)Unbound::nativeWindowNumber(draggedSource), (long long)Unbound::nativeWindowNumber(targetProgram), int(Unbound::nativeVisibilityFlags(targetProgram)));
            for (auto number : Unbound::nativeOrderedWindowNumbers()) for (auto window : QApplication::topLevelWidgets())
                if (Unbound::nativeWindowNumber(window) == number && window->frameGeometry().contains(dockPointer))
                    std::fprintf(stderr, "Pointer hit %s native=%lld visible=%d\n", qPrintable(window->objectName()), (long long)number, int(window->isVisible()));
        }
        check(attachmentPreview && attachmentPreview->isVisible() && dockTarget && dockTarget->isVisible() && dockTarget->width() >= 120,
            "panel attachment labels the selected region and previews the proposed split");
        const QPoint hover = targetProgram->frameGeometry().center() + QPoint(-65, -45);
        QMouseEvent hoverMove(QEvent::MouseMove, QPointF(), QPointF(hover), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(draggedSource, &hoverMove); settleFor(230);
        auto compass = draggedSource->findChild<QWidget *>("unboundDockCompass");
        if (!attachmentPreview || !attachmentPreview->isVisible()) {
            const auto cursor=QCursor::pos();
            std::fprintf(stderr,"Tab preview diagnostic requested=%d,%d cursor=%d,%d sourceVisible=%d targetVisible=%d buttons=%d grabber=%s popup=%s\n",
                hover.x(),hover.y(),cursor.x(),cursor.y(),int(draggedSource->isVisible()),int(targetProgram->isVisible()),int(QApplication::mouseButtons()),
                qPrintable(QWidget::mouseGrabber() ? QWidget::mouseGrabber()->objectName() : QString()),
                qPrintable(QApplication::activePopupWidget() ? QApplication::activePopupWidget()->objectName() : QString()));
            Unbound::flushDiagnostics(); QFile trace(logs.path()+"/events.jsonl");
            if (trace.open(QIODevice::ReadOnly)) for (const auto &line : trace.readAll().split('\n'))
                if (line.contains("dock.target.selection") || line.contains("drag.end")) std::fprintf(stderr,"%s\n",line.constData());
        }
        check((!compass || !compass->isVisible()) && attachmentPreview && attachmentPreview->isVisible(), "moving into the panel selects a regional tab preview without a delayed compass");
        QApplication::sendEvent(draggedSource, &move);
        QWidget covering;
        covering.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        covering.setObjectName("diagnosticCoveringWindow"); covering.setAttribute(Qt::WA_ShowWithoutActivating);
        covering.setGeometry(QRect(dockPointer - QPoint(30, 30), QSize(60, 60))); covering.show(); covering.raise(); settleFor(80);
        QApplication::sendEvent(draggedSource, &move);
        check((!dockTarget || !dockTarget->isVisible()) && (!attachmentPreview || !attachmentPreview->isVisible()), "covering non-dockable window blocks targets on panels behind it");
        covering.hide(); QApplication::sendEvent(draggedSource, &move);
        QMouseEvent up(QEvent::MouseButtonRelease, QPointF(100, 7), QPointF(press + delta), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(draggedSource, &up); settle(); settle();qApp->removeEventFilter(&syntheticMoves);
        check(source->window() == program->window() && source->window()->objectName().startsWith("unboundFloating_group_"),
            "releasing grip beside a panel creates an actual single-host group");
        Unbound::WorkspaceSettings groupSettings;
        check(groupSettings.data["groups"].toArray().size() == 1, "group identities and split geometry persist with workspace");
        }
        settle();
        {
            PanelControls restored; restored.show(); settleFor(600);
            check(source->window() == program->window() && source->window()->objectName().startsWith("unboundFloating_group_"),
                "workspace reload restores single-host group");
            Unbound::WorkspaceSettings groupedWorkspace;
            auto groupedValue = groupedWorkspace.data; groupedValue.remove("controls"); groupedValue.remove("selected_arrangement");
            Unbound::ArrangementStore groupedPresets;
            check(groupedPresets.create("Grouped viewers", groupedValue, groupedId).isEmpty(), "named preset stores group relationships");
            restored.findChild<QPushButton *>("group_2")->click(); settle();
            check(source->window() != program->window() && source->window() != &main && program->window() != &main,
                "Release Groups returns independent floating panels");
            restored.findChild<QPushButton *>("bulk_1")->click(); settle();
            check(source->parentWidget() == stack && program->parentWidget() == page, "Dock All restores original parents after grouping");
        }
        {
            PanelControls restored; restored.show(); settle();
            auto selector = restored.findChild<QComboBox *>("arrangements");
            selector->setCurrentIndex(selector->findData(groupedId));
            restored.findChild<QPushButton *>("restoreArrangement")->click(); settleFor(350);
            check(source->window() == program->window() && source->window()->objectName().startsWith("unboundFloating_group_"),
                "named preset restores group identities, orientation and split geometry after Dock All");
            restored.findChild<QPushButton *>("bulk_1")->click(); settle();
            Unbound::ArrangementStore cleanup;
            check(cleanup.remove(groupedId).isEmpty(), "grouped preset fixture cleanup");
        }
    }
    {
        QWidget page;
        auto layout = new QStackedLayout(&page);
        auto content = new QWidget(&page);
        layout->addWidget(content); page.show(); settle();
        Unbound::MacPanelSession session;
        check(!session.undock(content, &page, "Stack").isEmpty() && layout->currentWidget() == content &&
              content->parentWidget() == &page && !session.active(), "unsupported layout rejected unchanged");
    }
    {
        QWidget page;
        auto layout = new QVBoxLayout(&page);
        auto content = new QWidget(&page);
        layout->addWidget(content); page.show(); settle();
        { Unbound::MacPanelSession session; check(session.undock(content, &page, "Lifetime").isEmpty(), "lifetime setup"); }
        settle();
        check(layout->itemAt(0)->widget() == content && content->parentWidget() == &page, "session destruction restores content");
    }
    {
        qputenv("UNBOUND_WORKSPACE_FILE",(logs.path()+"/fresh-start.json").toUtf8());
        QFile stale(logs.path()+"/show-controls.request");
        check(stale.open(QIODevice::WriteOnly),"stale launch fixture opens"); stale.write("old-session"); stale.flush();
        check(stale.setFileTime(QDateTime::currentDateTimeUtc().addSecs(-60),QFileDevice::FileModificationTime),"stale launch fixture timestamp set"); stale.close();
        const auto keyBeforeStartup=Unbound::nativeKeyWindowNumber();
        Unbound::ControlService service(&app);
        check(Unbound::nativeKeyWindowNumber()==keyBeforeStartup,"background helper initialization does not change the native key window");
        PanelControls *controls = nullptr;
        for (auto window : QApplication::topLevelWidgets())
            if (window->objectName() == QLatin1String("unboundPanelTest")) controls = static_cast<PanelControls *>(window);
        check(controls != nullptr, "resident service creates controls");
        check(controls && !controls->isVisible(),"fresh helper startup leaves the Panel Manager closed");
        check(controls && !controls->findChild<QCheckBox *>("hideResolveBackground")->isChecked(),
            "fresh installation retains the native workspace without requiring a background checkbox");
        check(!QFile::exists(logs.path()+"/show-controls.request"),"startup consumes stale launch requests without reopening the manager");
        for (int i = 0; i < 2 && controls; ++i) {
            controls->hide();
            QSaveFile request(logs.path() + QStringLiteral("/show-controls.request"));
            check(request.open(QIODevice::WriteOnly), "launch request opened");
            request.write(QByteArray::number(i));
            check(request.commit(), "launch request committed");
            QElapsedTimer timer; timer.start();
            while (!controls->isVisible() && timer.elapsed() < 2500) { settle(); QThread::msleep(10); }
            check(controls->isVisible(), "relaunch request restores hidden controls");
            check(!QFile::exists(logs.path()+"/show-controls.request"),"explicit manager launch requests are single-use");
            int count = 0;
            for (auto window : QApplication::topLevelWidgets())
                if (window->objectName() == QLatin1String("unboundPanelTest")) ++count;
            check(count == 1, "relaunch reuses controls without duplicate sessions");
        }
    }
    {
        const auto path=logs.path()+"/startup-mode.json";
        qputenv("UNBOUND_WORKSPACE_FILE",path.toUtf8());
        const auto verifyStartupMode=[&](const QJsonObject &value,bool hidden) {
            QSaveFile file(path); check(file.open(QIODevice::WriteOnly),"startup preference fixture opens");
            file.write(QJsonDocument(value).toJson()); check(file.commit(),"startup preference fixture commits");
            prepareNativeFixture();PanelControls controls;
            check(controls.findChild<QCheckBox *>("hideResolveBackground")->isChecked()==hidden,
                "primary workspace ignores legacy modes and manual-hide policies");
        };
        verifyStartupMode({{"hide_background",true}},false);
        verifyStartupMode({{"native_workspace_mode","native"},{"hide_background",true}},false);
        verifyStartupMode({{"native_workspace_mode","floating"},{"hide_background",false}},false);
        verifyStartupMode({{"background_policy","manual-hidden"}},false);
        verifyStartupMode({{"background_policy","automatic"},{"hide_background",true}},false);
    }
    qputenv("UNBOUND_WORKSPACE_FILE",(logs.path()+"/Workspace.json").toUtf8());
    settle();
    {
        using Unbound::fitToDisplays;
        const QList<QRect> displays = {QRect(0, 24, 1440, 850), QRect(-1280, 0, 1280, 800)};
        check(displays[0].contains(fitToDisplays(QRect(4000, 3000, 600, 400), displays)), "disconnected display window rescued");
        check(displays[1].contains(fitToDisplays(QRect(-1200, 40, 600, 400), displays)), "negative display coordinates retained");
        check(fitToDisplays(QRect(0, 0, 5000, 5000), {displays[0]}) == displays[0], "oversized frame bounded to work area");
        check(fitToDisplays(QRect(50, -300, 400, 300), {displays[0]}).top() == 24, "title stays below menu bar");
        const QRect initial(50, 70, 400, 300);
        const QRect resized = Unbound::resizeFromEdge(initial, QPoint(1000, 1000), Qt::LeftEdge | Qt::TopEdge,
            QSize(160, 100), QSize(900, 700));
        check(resized.size() == QSize(160, 100) && resized.bottomRight() == initial.bottomRight(), "frameless resizing preserves opposite edge and minimum");
        check(Unbound::resizeFromEdge(initial, QPoint(2000, 2000), Qt::RightEdge | Qt::BottomEdge,
            QSize(160, 100), QSize(900, 700)).size() == QSize(900, 700), "frameless resizing respects maximum");
    }
    {
        QWidget page;
        auto pageLayout = new QVBoxLayout(&page);
        auto slot = new QWidget(&page);
        auto originalLayout = new QVBoxLayout(slot);
        auto header = new QLabel("Index tabs", slot);
        auto content = new QPushButton("Index row", slot);
        originalLayout->addWidget(header, 1); originalLayout->addWidget(content, 3);
        originalLayout->setContentsMargins(3, 4, 5, 6);
        pageLayout->addWidget(slot); page.show(); settle();
        int clicks = 0;
        QObject::connect(content, &QPushButton::clicked, [&clicks] { ++clicks; });
        Unbound::MacPanelSession session;
        for (auto chrome : {Unbound::WindowChrome::Standard, Unbound::WindowChrome::Compact, Unbound::WindowChrome::Hidden}) {
            session.setChrome(chrome);
            check(session.undockContents(slot, &page, "Index").isEmpty(), "multipart sidebar detaches");
            auto host = session.window(); settle();
            check(slot->isHidden(), "transferred content leaves a hidden native container rather than a duplicate blank surface");
            auto viewport=host->findChild<QScrollArea *>();
            check(slot->parentWidget() == &page && viewport && header->parentWidget() == viewport->widget() && content->parentWidget() == viewport->widget(),
                "native slot stays while all panel parts move together");
            const int flags = Unbound::nativeChromeFlags(host);
            check(chrome == Unbound::WindowChrome::Hidden ? !(flags & 1) : bool(flags & 1), "native title style matches option");
            check(chrome != Unbound::WindowChrome::Compact || (flags & 4), "compact style uses full-size title bar");
            if (chrome == Unbound::WindowChrome::Hidden) {
                const QSize before = host->size();
                const QPoint edge(host->width() - 2, host->height() - 2);
                const QPoint global = host->mapToGlobal(edge);
                QMouseEvent press(QEvent::MouseButtonPress, edge, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(host, &press);
                QMouseEvent move(QEvent::MouseMove, edge + QPoint(80, 60), global + QPoint(80, 60), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(host, &move);
                QMouseEvent release(QEvent::MouseButtonRelease, edge + QPoint(80, 60), global + QPoint(80, 60), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(host, &release);
                check(host->size() == before + QSize(80, 60), "hidden-title host resizes from edge mouse events");
            }
            content->click();
            session.setHiddenByUser(true); page.hide(); page.show(); settle();
            check(!host->isVisible() && session.hiddenByUser(), "user-hidden panel stays hidden across page return");
            session.setHiddenByUser(false); settle();
            check(host->isVisible(), "hidden panel can be recalled");
            check(session.dock().isEmpty() && slot->layout() == originalLayout && content->parentWidget() == slot &&
                originalLayout->stretch(1) == 3 && originalLayout->contentsMargins() == QMargins(3, 4, 5, 6),
                "sidebar restores exact layout and native ownership in every chrome mode");
            settle();
        }
        check(clicks == 3, "native content signals survive layout transfers");
    }
    {
        qputenv("UNBOUND_WORKSPACE_FILE", (logs.path() + QStringLiteral("/bulk-workspace.json")).toUtf8());
        QWidget main; main.setObjectName("UiMainWindow");
        auto mainLayout = new QVBoxLayout(&main);
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform"); mainLayout->addWidget(page);
        auto grid = new QGridLayout(page);
        const QStringList contentsNames = {"frameEditPageMediaPool", "frameEditPageEditIndex", "frameEditPageEffectsLibrary", "frameEditPageMetadata", "frameEditAudioMeter", "frameEditPageSoundLibrary", "frameEditPageKeyframeEditor"};
        QList<QWidget *> paneSlots; QList<QLayout *> originalLayouts;
        for (int i = 0; i < contentsNames.size(); ++i) {
            auto slot = new QWidget(page); slot->setObjectName(contentsNames[i]);
            auto box = new QVBoxLayout(slot); box->addWidget(new QLabel("Panel body", slot));
            grid->addWidget(slot, i / 3, i % 3); paneSlots.append(slot); originalLayouts.append(box);
        }
        const QStringList widgetNames = {"widgetEditInspector", "frameConformTimelines", "widgetEditTimelinePlayer"};
        for (int i = 0; i < widgetNames.size(); ++i) {
            auto widget = new QWidget(page); widget->setObjectName(widgetNames[i]); grid->addWidget(widget, 3, i);
        }
        auto stack = new QStackedWidget(page); auto source = new QWidget;
        source->setObjectName("widgetEditSourcePlayer"); stack->addWidget(source); grid->addWidget(stack, 4, 0);
        auto mediaToggle = new QPushButton("Media Pool", page);
        mediaToggle->setObjectName("btnEditToggleMediaPoolInSSM"); mediaToggle->setCheckable(true);
        grid->addWidget(mediaToggle, 4, 1); paneSlots[0]->hide();
        int toggleActivations = 0;
        QObject::connect(mediaToggle, &QPushButton::clicked, [&] { ++toggleActivations; paneSlots[0]->show(); });
        main.resize(900, 650); main.show(); settle();
        int exportRequests = 0;
        auto exportAction = new QAction(QString::fromUtf8("Quick Export…"), &main);
        exportAction->setObjectName("actionQuickExport");
        QObject::connect(exportAction, &QAction::triggered, [&] { ++exportRequests; });
        auto waitFor = [](const std::function<bool()> &done) {
            QElapsedTimer timer; timer.start();
            while (!done() && timer.elapsed() < 3000) { settle(); QThread::msleep(10); }
            return done();
        };
        auto hostCount = [] {
            int count = 0;
            for (auto widget : QApplication::topLevelWidgets())
                if (widget->objectName().startsWith("unboundFloating_")) ++count;
            return count;
        };
        {
            prepareNativeFixture();PanelControls controls; controls.show(); settle();
            controls.findChild<QPushButton *>("bulk_5")->click();
            check(exportRequests == 1, "Quick Export opens the native action without directly rendering");
            exportAction->setEnabled(false);
            controls.findChild<QPushButton *>("bulk_5")->click();
            check(exportRequests == 1, "unavailable Quick Export does not dispatch");
            controls.findChild<QPushButton *>("bulk_0")->click();
            check(waitFor([&] { return hostCount() == 11; }), "Undock All detaches eleven independent panels");
            check(toggleActivations == 1, "Undock All initializes a hidden native panel through its toggle");
            for (auto widget : QApplication::topLevelWidgets())
                if (widget->objectName() == "unboundFloating_widgetEditTimelinePlayer") widget->setGeometry(100, 120, 420, 280);
            controls.findChild<QPushButton *>("hide_widgetEditSourcePlayer")->click();
            controls.findChild<QCheckBox *>("pinControls")->setChecked(true);
            check(controls.windowFlags().testFlag(Qt::WindowStaysOnTopHint), "controls can win z-order");
            controls.close(); settle();
            check(hostCount() == 11 && !controls.isVisible(), "closing controls leaves floating workspace intact");
        }
        settle();
        check(hostCount() == 0, "shutdown docks live panels after saving desired state");
        for (int i = 0; i < paneSlots.size(); ++i) check(paneSlots[i]->layout() == originalLayouts[i], "bulk sidebars restore original layouts");
        {
            prepareNativeFixture();PanelControls controls; controls.show();
            check(waitFor([&] { return hostCount() == 11; }), "saved workspace restores eleven panels");
            check(controls.findChild<QCheckBox *>("pinControls")->isChecked(), "pin option persists");
            QWidget *sourceHost = nullptr, *programHost = nullptr;
            for (auto widget : QApplication::topLevelWidgets()) {
                if (widget->objectName() == "unboundFloating_widgetEditSourcePlayer") sourceHost = widget;
                if (widget->objectName() == "unboundFloating_widgetEditTimelinePlayer") programHost = widget;
            }
            check(sourceHost && !sourceHost->isVisible(), "hidden source state persists across workspace reload");
            check(programHost && programHost->size() == QSize(420, 280), "floating geometry persists across workspace reload");
            controls.findChild<QPushButton *>("bulk_2")->click(); settle();
            check(sourceHost && sourceHost->isVisible(), "Show All recalls hidden panels");
            controls.findChild<QPushButton *>("bulk_3")->click(); settle();
            bool allHidden = true;
            for (auto widget : QApplication::topLevelWidgets())
                if (widget->objectName().startsWith("unboundFloating_")) allHidden &= !widget->isVisible();
            check(allHidden, "Hide All retains but hides panels");
            controls.findChild<QPushButton *>("bulk_1")->click(); settle();
            check(hostCount() == 0 && source->parentWidget() == stack, "Dock All restores all panels including source slot");
            controls.findChild<QPushButton *>("bulk_0")->click();
            controls.findChild<QPushButton *>("bulk_1")->click();
            QElapsedTimer cancellation; cancellation.start();
            while (cancellation.elapsed() < 1100) { settle(); QThread::msleep(10); }
            check(hostCount() == 0, "Dock All cancels pending Undock All callbacks");
        }
        qputenv("UNBOUND_WORKSPACE_FILE", (logs.path() + QStringLiteral("/Workspace.json")).toUtf8());
    }
    {
        Unbound::WindowHierarchy hierarchy(&app);
        QWidget main; main.setObjectName("UiMainWindow");
        Unbound::MacFloatingHost timeline, pool, sound, source, program;
        timeline.setObjectName("unboundFloating_frameConformTimelines");
        pool.setObjectName("unboundFloating_frameEditPageMediaPool");
        sound.setObjectName("unboundFloating_frameEditPageSoundLibrary");
        source.setObjectName("unboundFloating_widgetEditSourcePlayer");
        program.setObjectName("unboundFloating_widgetEditTimelinePlayer");
        for (auto widget : QList<QWidget *>{&main, &timeline, &pool, &sound, &program, &source}) { widget->resize(300, 200); widget->show(); }
        settle();
        auto activate = [&](QWidget *widget) {
            widget->raise(); widget->activateWindow();
            QElapsedTimer timer; timer.start();
            while (Unbound::nativeKeyWindowNumber() != Unbound::nativeWindowNumber(widget) && timer.elapsed() < 1500) { settle(); QThread::msleep(10); }
            QEvent activation(QEvent::WindowActivate); QApplication::sendEvent(widget, &activation);
        };
        auto ahead = [](QWidget *front, QWidget *back) {
            QElapsedTimer committed; committed.start();
            do {
                const auto windows = Unbound::nativeOrderedWindowNumbers();
                const int a = windows.indexOf(Unbound::nativeWindowNumber(front));
                const int b = windows.indexOf(Unbound::nativeWindowNumber(back));
                if (a >= 0 && b >= 0 && a < b) return true;
                settle(); QThread::msleep(2);
            } while (committed.elapsed() < 250);
            return false;
        };
        activate(&source);
        qint64 key = Unbound::nativeKeyWindowNumber(); hierarchy.apply();
        check(key != 0 && key == Unbound::nativeWindowNumber(&source), "source has native keyboard focus before ordering");
        check(ahead(&source, &program) && ahead(&program, &pool) && ahead(&pool, &timeline) &&
              ahead(&sound, &timeline), "native hierarchy: active source above program, sidebars and timeline");
        check(Unbound::nativeKeyWindowNumber() == key, "native ordering preserves source keyboard focus");
        activate(&program); key = Unbound::nativeKeyWindowNumber(); hierarchy.apply();
        check(ahead(&program, &source) && ahead(&source, &pool) && Unbound::nativeKeyWindowNumber() == key,
            "active program wins viewer order without focus changes");
        activate(&timeline); key = Unbound::nativeKeyWindowNumber(); hierarchy.apply();
        check(key != 0 && key == Unbound::nativeWindowNumber(&timeline), "timeline has native keyboard focus before ordering");
        check(ahead(&source, &timeline) && ahead(&program, &timeline) && ahead(&pool, &timeline) &&
              Unbound::nativeKeyWindowNumber() == key, "timeline remains behind every panel while keeping keyboard focus");
        pool.hide(); hierarchy.apply();
        check(!pool.isVisible(), "hierarchy does not revive hidden panels");
        QWidget controls; controls.setObjectName("unboundPanelTest");
        controls.setWindowFlag(Qt::WindowStaysOnTopHint, true); controls.show(); settle(); hierarchy.apply();
        check(ahead(&controls, &source) && ahead(&controls, &program), "pinned controls stay above viewers");
        hierarchy.setEnabled(false);
        const auto natural = Unbound::nativeOrderedWindowNumbers(); hierarchy.apply();
        check(Unbound::nativeOrderedWindowNumbers() == natural, "disabled hierarchy does not reorder windows");
        hierarchy.setEnabled(true);
        QDialog dialog; dialog.setModal(true); dialog.show(); settle();
        const auto modalOrder = Unbound::nativeOrderedWindowNumbers(); hierarchy.apply();
        check(Unbound::nativeOrderedWindowNumbers() == modalOrder, "modal export/dialog order is respected");
        dialog.hide();
        QWidget relay; relay.setObjectName("relayWorkflowIntegration");
        auto field = new QLineEdit(&relay); auto relayLayout = new QVBoxLayout(&relay); relayLayout->addWidget(field);
        relay.show(); activate(&relay); field->setFocus(); settle(); key = Unbound::nativeKeyWindowNumber();
        hierarchy.apply();
        check(ahead(&relay, &source) && ahead(&relay, &timeline) && key == Unbound::nativeKeyWindowNumber() && QApplication::focusWidget() == field,
            "unmanaged Workflow window remains above workspace without losing text focus");
        QKeyEvent type(QEvent::KeyPress, Qt::Key_Q, Qt::NoModifier, "q"); QApplication::sendEvent(field, &type);
        check(field->text() == "q", "Workflow window retains text input after workspace ordering");
        QWidget group; group.setObjectName("unboundFloating_group_media_viewer"); group.show(); settle();
        pool.show(); activate(&pool); hierarchy.apply();
        check(ahead(&pool, &group), "inactive mixed group does not permanently dominate another sidebar");
        activate(&group); hierarchy.apply(); check(ahead(&group, &pool), "active group follows normal activation within its tier");
    }
    settle();
    {
        QWidget main; main.setObjectName("UiMainWindow"); auto layout=new QVBoxLayout(&main);
        auto page=new QWidget(&main); layout->addWidget(page); main.show(); page->show();
        QWidget ownerHost(&main,Qt::Tool); ownerHost.setObjectName("unboundFloating_timeline");
        auto owner=new QWidget(&ownerHost);auto ownerLayout=new QVBoxLayout(owner);(new QVBoxLayout(&ownerHost))->addWidget(owner);
        auto mixer=new QWidget(owner);mixer->setObjectName("frameEditAudioMeter");auto nativeLayout=new QVBoxLayout(mixer);nativeLayout->addWidget(new QPushButton("Mixer",mixer));ownerLayout->addWidget(mixer);
        ownerHost.show();settle();Unbound::MacPanelSession nested(nullptr);
        check(!nested.undockContents(mixer,page,"Mixer").isEmpty(),"nested Mixer is rejected without an explicit managed owner");
        check(nested.undockContents(mixer,page,"Mixer",owner).isEmpty(),"Mixer can detach from its managed floating Timeline");settle();
        nested.setHiddenByUser(true);check(nested.hiddenByUser(),"detached Mixer can be hidden");nested.setHiddenByUser(false);
        check(nested.dock().isEmpty() && mixer->layout()==nativeLayout,"Mixer docking returns its exact content layout to the retained owner slot");
    }
    Unbound::flushDiagnostics();
    QFile events(logs.path() + QStringLiteral("/events.jsonl"));
    check(events.open(QIODevice::ReadOnly), "diagnostic log created");
    int buttonEvents = 0, arrangementEvents = 0, verifiedDocks = 0;
    while (!events.atEnd()) {
        const auto record = QJsonDocument::fromJson(events.readLine()).object();
        check(record.contains("time") && record.contains("pid") && record.contains("build"), "diagnostic event identity");
        if (record["event"] == "button.click") ++buttonEvents;
        if (record["event"] == "window.move" || record["event"] == "window.resize") ++arrangementEvents;
        if (record["event"] == "panel.dock.verify" && record["passed"].toBool()) ++verifiedDocks;
    }
    check(buttonEvents >= 3 && arrangementEvents > 0 && verifiedDocks > 0, "button, arrangement and verified-dock events logged");
    if (!failures) std::puts("PASS: docking, eleven-panel workspaces, persistence, bulk cancellation, hide/show, viewport recovery, pinning, title styles, edge resizing, native z-order/focus, Quick Export, relaunch and logs");
    return failures ? 1 : 0;
}
