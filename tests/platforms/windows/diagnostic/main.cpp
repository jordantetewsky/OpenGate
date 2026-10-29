#include <QApplication>
#include <QTemporaryDir>
#include <QLibrary>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <QFile>
#include <qt_windows.h>
#include <cstdio>
#include <cstdlib>
#include "startupmode.h"
static void check(bool ok, const char *message)
{
    if (!ok) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    if (app.arguments().contains("--diagnostic-test")) {
        const auto option=app.arguments().at(app.arguments().indexOf("--diagnostic-test")+1);
        Unbound::StartupMode mode;
        check(Unbound::parseStartupMode(option.toStdWString(),mode), "Invalid diagnostic test mode");
        QTemporaryDir directory;
        check(directory.isValid(), "Cannot create diagnostic settings directory");
        const auto settingsPath=directory.filePath("Unbound.json");
        qputenv("UNBOUND_SETTINGS_FILE",settingsPath.toUtf8());
        QWidget main; main.setObjectName("UiMainWindow");
        auto mainLayout=new QVBoxLayout(&main);
        auto stack=new QStackedWidget(&main); stack->setObjectName("widgetStack_Panel"); mainLayout->addWidget(stack);
        auto page=new QWidget; page->setObjectName("WStackPage_Conform"); stack->addWidget(page);
        auto root=new QWidget(page); root->setObjectName("frameEditPageWidgetsContainer");
        auto pane=new QWidget(root); pane->setObjectName("frameEditPageMediaPool");
        auto layout=new QVBoxLayout(pane); layout->addWidget(new QPushButton("Native content",pane));
        main.show(); app.processEvents();
        QLibrary helper(app.arguments().last()); helper.setLoadHints(QLibrary::PreventUnloadHint);
        check(helper.load(),qPrintable(helper.errorString()));
        auto start=reinterpret_cast<DWORD (WINAPI *)(void *)>(helper.resolve("UnboundStart"));
        check(start && start(reinterpret_cast<void *>(999))==0, "Invalid bootstrap mode accepted");
        if (mode != Unbound::StartupMode::LoadOnly) {
            auto argument=reinterpret_cast<void *>(static_cast<ULONG_PTR>(mode));
            check(start(argument)==1 && start(argument)==1, "Diagnostic bootstrap failed");
        }
        app.processEvents();
        auto controller=app.findChild<QObject *>("UnboundPaneController");
        const bool passive=mode==Unbound::StartupMode::LoadOnly || mode==Unbound::StartupMode::BootstrapOnly;
        check(bool(controller)!=passive, "Unexpected controller presence");
        const bool decorated=!passive && mode!=Unbound::StartupMode::NoDecorations;
        check(!pane->findChildren<QToolButton *>("resolvePaneUndockButton").isEmpty()==decorated,
            "Unexpected diagnostic decoration state");
        if (controller) {
            bool polling=false;
            for (auto timer:controller->findChildren<QTimer *>())
                if (timer->interval()==1000 && timer->isActive()) polling=true;
            check(!polling, "Recurring discovery timer must not run in any mode");
        }
        delete controller; // Flush placement while fixture widgets still exist.
        check(QFile::exists(settingsPath)==(!passive && mode!=Unbound::StartupMode::NoPlacement),
            "Unexpected placement persistence in diagnostic mode");
        std::puts("PASS: diagnostic bootstrap, component isolation and placement persistence");
        return 0;
    }
    return 1;
}
