#include "native-fixture-start.h"
// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QThread>
#include <cstdio>
#include "panelcontrols.h"
static bool waitFor(const std::function<bool()> &ready,int timeout=3500) {
    QElapsedTimer clock; clock.start();
    do { QApplication::processEvents(); QThread::msleep(2); if (ready()) return true; } while (clock.elapsed()<timeout);
    return false;
}
static void settle(int duration=350) {
    QElapsedTimer clock; clock.start(); while (clock.elapsed()<duration) { QApplication::processEvents(); QThread::msleep(2); }
}
static QJsonObject read(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject(); }
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    if (argc!=3 && argc!=4) return 2;
    const bool attached=argc==4 && QString::fromLocal8Bit(argv[3])=="attached";
    const bool writing=QString::fromLocal8Bit(argv[1])=="write";
    const auto expectedPath=QString::fromLocal8Bit(argv[2]);
    int failures=0; const auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    if (writing) {
        Unbound::ArrangementStore store; QString selected;
        check(store.create("Resume fixture",{{"page","edit"},{"version",1}},selected).isEmpty(),"create selected preset identity");
        Unbound::WorkspaceSettings initial; initial.data={{"selected_arrangement",selected},{"native_workspace_mode","native"}}; check(initial.save(),"write initial fixture settings");
    }
    QWidget main; main.setObjectName("UiMainWindow"); main.resize(1100,760);
    auto outer=new QVBoxLayout(&main); auto page=new QWidget(&main); page->setObjectName("WStackPage_Conform"); outer->addWidget(page);
    auto native=new QGridLayout(page);
    const QStringList names{"widgetEditInspector","frameConformTimelines","widgetEditSourcePlayer","widgetEditTimelinePlayer","frameEditPageMediaPool"};
    QList<QWidget *> panes;
    for (int i=0;i<names.size();++i) {
        auto pane=new QWidget(page); pane->setObjectName(names[i]); auto layout=new QVBoxLayout(pane);
        auto content=new QPushButton(names[i],pane); layout->addWidget(content); native->addWidget(pane,i/3,i%3);
        // Media Pool transfers its content layout; its original slot stays in
        // Resolve as a vacated placeholder. Observe real content, not that slot.
        panes.append(i==4 ? static_cast<QWidget *>(content) : pane);
    }
    auto footer=new QWidget(&main); footer->setObjectName("m_PageSwitcherContainer");
    auto footerLayout=new QHBoxLayout(footer); footerLayout->addWidget(new QPushButton("Edit",footer)); outer->addWidget(footer);
    auto nativeEdit=footer->findChild<QPushButton *>();nativeEdit->setObjectName("buttonConformPanel");
    auto showPanelMenu=new QMenu("Show Panel in Workspace",&main); showPanelMenu->setObjectName("workspaceShowPanel");
    QAction viewer("Single Viewer Mode",&main); viewer.setObjectName("workspaceSingleViewerModeToggle"); viewer.setCheckable(true);
    main.show(); auto controls=new PanelControls;
    const auto click=[&](const QString &name) { auto button=controls->findChild<QPushButton *>(name); check(button!=nullptr,"fixture control exists"); if (button) button->click(); };
    if (writing) {
        settle(); for (const auto &name : names) click("undock_"+name);
        check(waitFor([&] { for (auto pane : panes) if (pane->window()==&main) return false; return true; }),"all fixture panes detach");
        auto join=[&](int a,int b,const QString &operation) {
            controls->findChild<QComboBox *>("groupFirst")->setCurrentIndex(a);
            controls->findChild<QComboBox *>("groupSecond")->setCurrentIndex(b); click(operation);
            check(waitFor([&] { return panes[a]->window()==panes[b]->window(); }),"group preparation completes"); settle();
        };
        join(2,4,"group_3"); join(2,3,"group_1");
        auto group=dynamic_cast<Unbound::MacFloatingHost *>(panes[2]->window()); check(group!=nullptr,"native group host exists");
        if (group) {
            group->setGeometry(80,100,1000,650); group->setPinned(true); settle();
            if (auto splitter=group->findChild<QSplitter *>()) splitter->setSizes({430,180});
        }
        panes[1]->window()->setGeometry(100,160,760,440);
        auto inspector=dynamic_cast<Unbound::MacFloatingHost *>(panes[0]->window());
        if (inspector) { inspector->setGeometry(220,170,450,500); inspector->setPinned(true); }
        Unbound::FooterPanel *navigation=nullptr;
        for(auto object:controls->children())if(auto model=dynamic_cast<Unbound::FooterPanel *>(object))navigation=model;
        check(navigation!=nullptr,"surface navigation model is available");
        if(navigation){if(attached)navigation->setState({{"surfaces",QJsonObject{{panes[0]->window()->objectName(),false}}}});navigation->restore();}
        check(navigation&&navigation->pinned(panes[0]->window()),"legacy per-surface pin choice does not override reserved-band visibility");
        check(waitFor([&] {return footer->window()==&main;}),"primary main workspace keeps the complete original native footer while panels float");
        auto footerAction=showPanelMenu->findChild<QAction *>("unboundPageNavigation");
        check(footerAction!=nullptr,"footer remains discoverable in native Show Panel menu");
        if (footerAction) footerAction->trigger();
        check(waitFor([&] { return !footer->isVisible(); }),"menu hides the pinned footer without docking it");
        click("hide_frameEditPageMediaPool"); click("show_widgetEditSourcePlayer"); settle(550);
        delete controls; controls=nullptr;
        const auto saved=read(qEnvironmentVariable("UNBOUND_WORKSPACE_FILE"));
        const auto footerState=saved["footer"].toObject();
        check(!footerState["floating"].toBool()&&footerState["hidden"].toBool()&&footerState["model"]=="reserved-native-bands-v2"&&!footerState.contains("surfaces"),
            "last view records one global footer visibility without independent pin choices");
        check(saved["groups"].toArray().size()==1 && saved["selected_arrangement"].toString().isEmpty(),"Native-origin rearrangement persists as an unnamed view rather than overwriting an unrelated preset");
        QFile expected(expectedPath); check(expected.open(QIODevice::WriteOnly),"open cross-process expectation"); expected.write(QJsonDocument(saved).toJson());
    } else {
        const auto expected=read(expectedPath); check(!expected.isEmpty(),"previous process wrote expectations");
        check(waitFor([&] { return panes[2]->window()!=&main && panes[2]->window()==panes[3]->window() && panes[2]->window()==panes[4]->window() && panes[0]->window()!=&main && panes[1]->window()!=&main; }),"fresh process restores groups and independent windows automatically");
        check(footer->window()==&main,"hidden footer retains a reversible native or primary-surface slot");
        settle(650);
        const auto actual=read(qEnvironmentVariable("UNBOUND_WORKSPACE_FILE"));
        check(actual["footer"]==expected["footer"]&&!footer->isVisible(),"fresh process restores shared visibility and surface placement");
        check(!footer->isVisible(),"restored original footer starts globally hidden");
        auto footerAction=showPanelMenu->findChild<QAction *>("unboundPageNavigation");
        check(footerAction!=nullptr,"restored hidden footer still has a native menu caller");
        if (footerAction) footerAction->trigger();
        check(waitFor([&] { return footer->isVisible(); }),"native menu retrieves hidden Page Navigation after restart");
        check(waitFor([&] {return footer->window()==&main && footer->isVisible();}),"native menu restores the complete original footer on the primary surface after restart");
        check(actual["groups"]==expected["groups"],"fresh process restores exact group frame, pin, nested tree, tabs and divider proportions");
        check(actual["selected_arrangement"]==expected["selected_arrangement"],"selected workspace identity survives restart");
        for (const auto &name : {names[0],names[1]}) {
            const auto before=expected["panels"].toObject()[name].toObject(),after=actual["panels"].toObject()[name].toObject();
            check(Unbound::WorkspaceSettings::rectangle(before)==Unbound::WorkspaceSettings::rectangle(after) && before["pinned"]==after["pinned"],"independent panel geometry and pin survive restart");
        }
        check(actual["panels"].toObject()[names[4]].toObject()["hidden"].toBool() && !panes[4]->isVisible() && panes[2]->isVisible(),"hidden Media Pool door stays hidden while Source is restored");
        delete controls;
    }
    Unbound::flushDiagnostics();
    if (!failures) std::puts(writing ? "PASS: saved last modified view before process exit" : "PASS: fresh-process automatic last-view restoration");
    return failures ? 1 : 0;
}
