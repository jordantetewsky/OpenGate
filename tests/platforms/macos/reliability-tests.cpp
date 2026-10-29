#include <QApplication>
#include <QPushButton>

#include <QElapsedTimer>
#include <QLineEdit>
#include <QShortcut>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelsession.h"
#include "interactiondiagnostics.h"
#include "activationbridge.h"
#include "windowhierarchy.h"

class TestApplication : public QApplication
{
public:
    using QApplication::QApplication;
    QPointer<QObject> watched;
    int notifications = 0;
    bool notify(QObject *object, QEvent *event) override
    {
        if (object == watched && event->type() == QEvent::WindowActivate) ++notifications;
        return QApplication::notify(object, event);
    }
};
class ActivationRecorder : public QObject
{
public:
    int received = 0;
    bool eventFilter(QObject *, QEvent *event) override
    { if (event->type() == QEvent::WindowActivate) ++received; return false; }
};

static int failures = 0;
static void check(bool value, const char *description)
{
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", description); }
}
static void settle(int duration = 35)
{
    QElapsedTimer timer; timer.start();
    do { QCoreApplication::processEvents(); QThread::msleep(1); } while (timer.elapsed() < duration);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
static void activate(QWidget *widget)
{
    widget->show(); Unbound::activateNativeWindowForTest(widget); widget->activateWindow();
    QElapsedTimer timer; timer.start();
    while ((Unbound::nativeKeyWindowNumber() != Unbound::nativeWindowNumber(widget) || QApplication::activeWindow() != widget) && timer.elapsed() < 1500) {
        settle(); QThread::msleep(10);
    }
}
static void pressB(QWidget *widget)
{
    QKeyEvent down(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, "b");
    QApplication::sendEvent(widget, &down);
    QKeyEvent up(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, "b");
    QApplication::sendEvent(widget, &up);
}

int main(int argc, char **argv)
{
    TestApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    {
        QWidget main; main.setObjectName("UiMainWindow"); main.resize(600,400);
        auto layout = new QVBoxLayout(&main); auto edit = new QLineEdit(&main); layout->addWidget(edit);
        QWidget controls(nullptr, Qt::Window | Qt::WindowStaysOnTopHint); controls.setObjectName("unboundPanelTest");
        QWidget viewer; viewer.setObjectName("UiCinemaViewerDialog"); viewer.resize(600,400);
        main.show(); controls.show(); main.activateWindow(); edit->setFocus(); settle();
        const auto initialLevel = Unbound::nativeWindowLevelForTest(&viewer);
        Unbound::WindowHierarchy hierarchy(nullptr);
        viewer.show(); settle(150);
        std::printf("Cinema level=%lld controls=%lld state=%d\n", (long long)Unbound::nativeWindowLevelForTest(&viewer), (long long)Unbound::nativeWindowLevelForTest(&controls), int(QGuiApplication::applicationState()));
        check(Unbound::visibleCinemaViewer() == &viewer &&
            Unbound::nativeWindowLevelForTest(&viewer) > Unbound::nativeWindowLevelForTest(&controls),
            "Cinema viewer foreground tier exceeds pinned controls while visible");
        hierarchy.apply(); settle();
        check(Unbound::nativeKeyWindowNumber() == Unbound::nativeWindowNumber(&viewer),
            "normal panel ordering does not steal the cinema key window");
        viewer.hide(); settle(120);
        check(Unbound::nativeWindowLevelForTest(&viewer) == initialLevel,
            "Cinema exit restores its original native window level");
    }
    QTemporaryDir logs;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", logs.path().toUtf8());
    Unbound::InteractionDiagnostics diagnostics(&app);
    {
        QWidget main; main.resize(600,400); auto layout = new QVBoxLayout(&main);
        auto page = new QWidget(&main); layout->addWidget(page); auto row = new QVBoxLayout(page);
        auto player = new QWidget(page); player->setObjectName("widgetEditTimelinePlayer"); row->addWidget(player);
        main.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(player,page,"Program Monitor").isEmpty(), "native viewer recovery fixture starts floating");
        QWidget cinema; cinema.setObjectName("UiCinemaViewerDialog"); cinema.show(); player->hide(); settle(240);
        check(player->isHidden(), "panel recovery yields while Cinema deliberately hides the native player");
        cinema.hide(); settle(240);
        check(player->isVisible() && session.window()->isAncestorOf(player) && player->window()==session.window()->window(),
            "Cinema exit resumes native player recovery in its retained floating host");
        check(session.dock().isEmpty(), "Cinema recovery leaves original native docking intact");
    }
    {
        QWidget main; main.setObjectName("UiMainWindow");
        auto pages = new QStackedWidget(&main);
        auto mainLayout = new QVBoxLayout(&main); mainLayout->addWidget(pages);
        auto edit = new QWidget; edit->setObjectName("WStackPage_Conform");
        auto deliver = new QWidget;
        pages->addWidget(edit); pages->addWidget(deliver);
        auto holder = new QWidget(edit); holder->setObjectName("frameEditTimelinesHolder");
        auto editLayout = new QVBoxLayout(edit); editLayout->addWidget(holder);
        auto original = new QVBoxLayout(holder);
        auto timeline = new QWidget(holder); timeline->setObjectName("frameConformTimelines");
        original->addWidget(timeline);
        auto foreign = new QWidget(deliver); foreign->setObjectName("m_TimeLineWidget");
        auto deliverLayout = new QVBoxLayout(deliver); deliverLayout->addWidget(foreign);
        main.resize(800, 500); main.show(); settle();
        Unbound::MacPanelSession session;
        check(session.undock(timeline, edit, "Timeline").isEmpty(), "timeline setup");
        QPointer<QWidget> host = session.window();
        const QRect arrangement = host->geometry();
        for (int cycle = 0; cycle < 10; ++cycle) {
            pages->setCurrentWidget(deliver);
            timeline->setParent(foreign); timeline->hide(); settle();
            check(host && !host->isVisible(), "timeline host hides off Edit");
            pages->setCurrentWidget(edit); settle();
            check(host && host->isAncestorOf(timeline) && timeline->window()==host->window() && timeline->isVisible() && host->isVisible(),
                "native page reparent is recovered without Dock All");
            check(host && host->geometry() == arrangement, "page return preserves floating geometry");
        }
        session.setHiddenByUser(true);
        pages->setCurrentWidget(deliver); timeline->setParent(foreign); timeline->hide();
        pages->setCurrentWidget(edit); settle();
        check(host && !host->isVisible(), "explicit hidden choice survives page recovery");
        session.setHiddenByUser(false);
        check(session.dock().isEmpty() && timeline->parentWidget() == holder, "recovered timeline docks");
    }
    settle();
    {
        QWidget page; auto original = new QVBoxLayout(&page); auto content = new QWidget(&page); original->addWidget(content);
        page.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(content, &page, "Changed layout").isEmpty(), "layout replacement setup");
        auto placeholder = original->itemAt(0)->widget();
        delete original;
        auto current = new QGridLayout(&page); current->addWidget(placeholder, 2, 1, 2, 3);
        check(session.dock().isEmpty() && content->parentWidget() == &page && current->indexOf(content) >= 0,
            "Dock All uses the current native placeholder layout");
        int row, column, rows, columns;
        current->getItemPosition(current->indexOf(content), &row, &column, &rows, &columns);
        check(row == 2 && column == 1 && rows == 2 && columns == 3, "native replacement layout span is retained");
    }
    settle();
    {
        QWidget page, external;
        auto original = new QVBoxLayout(&page); auto content = new QWidget(&page); original->addWidget(content);
        page.show(); external.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(content, &page, "External owner").isEmpty(), "external-owner setup");
        content->setParent(&external); content->show(); settle();
        check(!session.dock().isEmpty() && content->parentWidget() == &external,
            "recovery does not steal content from an unrelated native window");
    }
    settle();
    {
        QWidget main;
        auto pages = new QStackedWidget(&main); auto box = new QVBoxLayout(&main); box->addWidget(pages);
        auto edit = new QWidget; auto deliver = new QWidget; pages->addWidget(edit); pages->addWidget(deliver);
        auto holder = new QWidget(edit); auto editBox = new QVBoxLayout(edit); editBox->addWidget(holder);
        auto original = new QVBoxLayout(holder); auto timeline = new QWidget(holder); original->addWidget(timeline);
        auto foreign = new QWidget(deliver); auto deliverBox = new QVBoxLayout(deliver); deliverBox->addWidget(foreign);
        main.show(); settle();
        Unbound::MacPanelSession session;
        check(session.undock(timeline, edit, "Timeline").isEmpty(), "off-page dock setup");
        pages->setCurrentWidget(deliver); timeline->setParent(foreign); timeline->show(); settle();
        check(session.dock().isEmpty(), "dock request accepted during native page ownership");
        check(timeline->parentWidget() == foreign, "off-page dock does not steal native timeline");
        pages->setCurrentWidget(edit); settle();
        check(!session.active() && timeline->parentWidget() == holder && original->indexOf(timeline) >= 0,
            "deferred docking finishes when Edit returns");
    }
    settle();
    {
        QWidget page; auto original = new QVBoxLayout(&page); auto content = new QWidget(&page); original->addWidget(content);
        page.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(content, &page, "Reclaimed").isEmpty(), "reclaim setup");
        original->addWidget(content); content->show();
        check(session.dock().isEmpty() && !session.active() && original->count() == 1 && original->itemAt(0)->widget() == content,
            "Dock All tolerates native restoration before our request");
    }
    settle();
    {
        QWidget main; auto page = new QWidget(&main); auto mainBox = new QVBoxLayout(&main); mainBox->addWidget(page);
        auto row = new QHBoxLayout(page); auto source = new QWidget(page); auto program = new QWidget(page);
        source->setObjectName("widgetEditSourcePlayer"); program->setObjectName("widgetEditTimelinePlayer");
        row->addWidget(source); row->addWidget(program);
        auto body = new QLabel("Program frame", program); auto layout = new QVBoxLayout(program); layout->addWidget(body);
        auto alternate = new QLabel("Intentionally hidden alternate", program); layout->addWidget(alternate); alternate->hide();
        main.show(); settle(); Unbound::MacPanelSession sourceSession, programSession;
        check(sourceSession.undock(source, page, "Source").isEmpty() && programSession.undock(program, page, "Program").isEmpty(), "viewer setup");
        activate(sourceSession.window()); const qint64 key = Unbound::nativeKeyWindowNumber();
        program->hide(); settle();
        check(program->isVisible() && body->isVisible() && alternate->isHidden(), "native root hide is recovered without exposing alternate views");
        check(Unbound::nativeKeyWindowNumber() == key, "viewer recovery preserves Source keyboard focus");
    }
    settle();
    {
        QWidget main; main.setObjectName("UiMainWindow");
        auto page = new QWidget(&main); auto mainBox = new QVBoxLayout(&main); mainBox->addWidget(page);
        auto root = new QWidget(page); auto pageBox = new QVBoxLayout(page); pageBox->addWidget(root);
        auto editor = new QWidget(root); editor->setObjectName("m_TiTimelineView"); editor->setFocusPolicy(Qt::StrongFocus);
        auto field = new QLineEdit(root); auto rootBox = new QVBoxLayout(root); rootBox->addWidget(editor); rootBox->addWidget(field);
        int rippleDelete = 0; QShortcut command(QKeySequence("B"), &main);
        QObject::connect(&command, &QShortcut::activated, [&] { ++rippleDelete; });
        main.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(root, page, "Timeline").isEmpty(), "shortcut setup");
        activate(session.window()); editor->setFocus(Qt::MouseFocusReason); settle();
        check(QApplication::focusWidget() == editor, "timeline editing widget holds keyboard focus");
        pressB(editor); check(rippleDelete == 1, "main-window B shortcut fires exactly once in floating timeline");
        field->setFocus(); pressB(field);
        check(field->text() == "b" && rippleDelete == 1, "text entry keeps B instead of deleting timeline selection");
    }
    settle();
    {
        QWidget main; main.setObjectName("UiMainWindow");
        auto page = new QWidget(&main); page->setObjectName("WStackPage_Conform");
        auto mainLayout = new QVBoxLayout(&main); mainLayout->addWidget(page);
        auto editor = new QLineEdit(page); auto pageLayout = new QVBoxLayout(page); pageLayout->addWidget(editor);
        main.show(); settle(); Unbound::MacPanelSession session;
        check(session.undock(editor, page, "Timeline").isEmpty(), "activation scope setup");
        activate(session.window()); editor->setFocus(); settle();
        ActivationRecorder mainEvents, hostEvents;
        main.installEventFilter(&mainEvents); session.window()->installEventFilter(&hostEvents);
        Unbound::ResolveActivationBridge bridge(&app);
        app.watched = &main; app.notifications = 0;
        const auto focus = QApplication::focusWidget(); const auto key = Unbound::nativeKeyWindowNumber();
        for (int i = 0; i < 2; ++i) {
            QEvent activation(QEvent::WindowActivate); QApplication::sendEvent(session.window(), &activation);
        }
        check(app.notifications == 2 && mainEvents.received == 0 && hostEvents.received == 2,
            "page scope notification reaches application once per activation without fake activation reaching main widgets");
        check(QApplication::focusWidget() == focus && Unbound::nativeKeyWindowNumber() == key && QApplication::activeWindow() == session.window(),
            "scope bridge preserves real text focus, Qt active host and native key window");
        page->hide(); settle(); const int before = app.notifications;
        QEvent offPage(QEvent::WindowActivate); QApplication::sendEvent(session.window(), &offPage);
        check(app.notifications == before, "scope bridge skips inactive Edit page");
        app.watched = nullptr;
    }
    {
        QWidget controls; controls.setObjectName("unboundPanelTest");
        QPushButton button("Diagnostic activation", &controls);
        button.setObjectName("diagnosticActivation"); controls.show(); button.show(); settle();
        button.click();
        QAction action("Diagnostic menu action", &controls);
        action.setObjectName("diagnosticMenuAction"); controls.addAction(&action); action.trigger();
        QAction cinema("Cinema Viewer", &controls);
        cinema.setObjectName("workspaceViewerModeCinemaViewer"); controls.addAction(&cinema); cinema.trigger();
    }
    settle();
    Unbound::flushDiagnostics();
    QFile events(logs.path() + QStringLiteral("/events.jsonl"));
    check(events.open(QIODevice::ReadOnly), "interaction evidence log exists");
    bool shortcutObserved = false, textObserved = false, pageMoveObserved = false, buttonObserved = false, actionObserved = false, cinemaObserved = false;
    while (!events.atEnd()) {
        const auto record = QJsonDocument::fromJson(events.readLine()).object();
        if (record["event"] == "input.button.clicked" && record["button"].toObject()["name"] == "diagnosticActivation") buttonObserved = true;
        if (record["event"] == "input.action.triggered" && record["action"] == "diagnosticMenuAction") actionObserved = true;
        if (record["event"] == "viewer.cinema.context" && record["command"] == "workspaceViewerModeCinemaViewer" && record["phase"] == "command-triggered") cinemaObserved = true;
        check(record["component"] == "OpenGate" && record["module"] == "UnboundWidgetProbe.dylib", "logs identify OpenGate and its actual binary module");
        if (record["event"] == "input.shortcut") shortcutObserved = true;
        if (record["event"] == "panel.native-change") pageMoveObserved = true;
        if (record["event"] == "input.key.observed" && record["text_editing"].toBool()) {
            textObserved = true;
            check(!record.contains("key") && !record.contains("text"), "typed text and key values are excluded from diagnostics");
        }
    }
    check(cinemaObserved, "native Cinema Viewer dispatch has a separate trace from panel maximize");
    check(buttonObserved && actionObserved, "control button activation and shortcut-free menu actions logged");
    check(shortcutObserved && textObserved && pageMoveObserved, "logs distinguish shortcut delivery, text input and native page movement");
    if (!failures) std::puts("PASS: native page moves, viewer visibility, docking recovery and owned-window shortcut scope");
    return failures ? 1 : 0;
}
