// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QStyle>
#include <QPushButton>
#include "floatinghost.h"
#include "compacttoolbar.h"
#include "toolbardoors.h"
#include "paneloverflow.h"
#include <QElapsedTimer>
#include <QThread>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include "workspaceheaders.h"
#include "panelcallermenu.h"
#include <cstdio>
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    QWidget main,first(&main,Qt::Tool),second(&main,Qt::Tool),toolbar(&main),center(&toolbar);
    first.setObjectName("unboundFloating_primary");second.setObjectName("unboundFloating_secondary");
    first.setWindowTitle("Timeline");second.setWindowTitle("Source Monitor");
    for (auto widget:{&main,&first,&second}) { widget->setAttribute(Qt::WA_DontShowOnScreen);widget->resize(600,400);widget->show(); }
    center.setObjectName("UiProjectsMainBarImp");auto project=new QPushButton("Project A",&center);project->setObjectName("btnProjectsMainBarProjectList");
    (new QVBoxLayout(&center))->addWidget(project);(new QVBoxLayout(&toolbar))->addWidget(&center);toolbar.show();center.show();project->show();
    double scale=1.;int floated=0;
    bool source=true;int calls=0;QMenu global;auto globalAction=global.addAction("Show Source Monitor");
    Unbound::WorkspaceHeaders headers(nullptr);
    headers.primary=[&] { return &first; };headers.projectSurface=[&] { return &toolbar; };
    headers.add({"timeline","Timeline",[&] { return &first; },[] { return true; },[](bool) {}});
    headers.add({"source","Source Monitor",[&] { return &second; },[&] { return source; },[&](bool shown) { source=shown;++calls; }});
    headers.add({"unboundWorkspaceToolbar","Workspace Toolbar",[&] { return &first; },[] { return true; },[](bool) {},
        [&] { return scale; },[&](double value) { scale=value; },[&] { ++floated; }});
    auto settle=[] { QElapsedTimer t;t.start();while (t.elapsed()<90) { QApplication::processEvents();QThread::msleep(1); } };
    headers.schedule();settle();bool ok=first.windowTitle()=="Timeline" && second.windowTitle()=="Source Monitor";
    ok &= !Unbound::nativeHeaderMenuFrameForTest(&main).isEmpty()&&Unbound::nativeHeaderMenuFrameForTest(&first).isEmpty()&&Unbound::nativeHeaderMenuFrameForTest(&second).isEmpty();
    toolbar.hide();headers.schedule();settle();ok &= !Unbound::nativeHeaderMenuFrameForTest(&main).isEmpty();toolbar.show();
    project->hide();project->setText("Project B");headers.schedule();settle();do { const bool passed=(first.windowTitle()=="Timeline"); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    QMenu local;headers.populate(&local,&second);do { const bool passed=(local.actions().size()==3 && local.actions()[1]->text()=="Source Monitor"); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    local.actions()[1]->trigger();settle();do { const bool passed=(!source && calls==1); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    headers.panelEntries=[&](QMenu *menu){menu->addAction(globalAction);};
    QMenu primary;headers.populate(&primary,&first);QMenu secondary;headers.populate(&secondary,&second);
    ok &= primary.actions().contains(globalAction)&&secondary.actions().contains(globalAction);
    for(auto action:primary.actions())ok &= action->text()!="All Panels" && action->text()!="This Window";
    settle();ok &= floated==0 && qAbs(scale-1.)<.001;
    Unbound::PanelCallerMenu registry(nullptr);registry.currentPage=[] {return QStringLiteral("edit");};
    bool shown=true;int visibilityCalls=0,showOnlyCalls=0;
    registry.add({"source","Source Monitor","edit",[]{return true;},[&]{return shown;},[&]{shown=true;++showOnlyCalls;},[&](bool value){shown=value;++visibilityCalls;}});
    registry.add({"inspector","Inspector","edit",[]{return true;},[]{return false;},[]{}});
    QMenu flat;registry.populateFlat(&flat);ok &= flat.actions().size()==2&&flat.actions().first()->isChecked();
    for(auto action:flat.actions())ok &= !action->menu();
    flat.actions().first()->trigger();settle();ok &= !shown&&visibilityCalls==1&&showOnlyCalls==0;
    QMenu other;registry.populateFlat(&other);ok &= !other.actions().first()->isChecked();other.actions().first()->trigger();settle();ok &= shown&&visibilityCalls==2;
    QMenu originalRoot;auto originalMenu=registry.wire(&originalRoot);auto originalAction=originalMenu->findChild<QAction *>("openGateShowPanel_source",Qt::FindDirectChildrenOnly);ok &= originalMenu->findChildren<QMenu *>().isEmpty();originalAction->trigger();settle();ok &= !shown&&visibilityCalls==3&&showOnlyCalls==0;
    QWidget compactWrapper;QStackedWidget stack(&compactWrapper);QWidget toolbarRow;
    toolbarRow.setFixedHeight(30);stack.addWidget(&toolbarRow);Unbound::CompactToolbar fit(&compactWrapper,&stack);settle();
    do { const bool passed=(compactWrapper.maximumHeight()==30); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    toolbarRow.setFixedHeight(24);QEvent relayout(QEvent::LayoutRequest);QApplication::sendEvent(&stack,&relayout);settle();
    do { const bool passed=(compactWrapper.maximumHeight()==24); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    Unbound::MacFloatingHost chrome(&main);chrome.setAttribute(Qt::WA_DontShowOnScreen);chrome.setChrome(Unbound::WindowChrome::Standard);
    do { const bool passed=(chrome.contentsMargins().top()==0); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    chrome.setChrome(Unbound::WindowChrome::Compact);chrome.show();settle();
    ok &= Unbound::setNativeHeaderMenu(&chrome,true,[] {});
    Unbound::MacFloatingHost strip(&main);strip.setAttribute(Qt::WA_DontShowOnScreen);strip.setProperty("unboundToolbarStrip",true);
    strip.setChrome(Unbound::WindowChrome::Standard);strip.show();settle();
    do { const bool passed=(strip.contentsMargins().top()==0 && strip.contentsMargins().left()>=100); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    ok &= Unbound::setNativeHeaderMenu(&strip,true,[] {});
    QWidget doorSurface;QWidget doorRow(&doorSurface);doorRow.setObjectName("frameEditTopBarLeftInSSM");new QHBoxLayout(&doorRow);
    bool doorShown=false;int doorCalls=0;Unbound::ToolbarDoors doors(nullptr);
    doors.add({"source","Source Monitor",[&] { return doorShown; },[&] { doorShown=!doorShown;++doorCalls; }});
    doors.wire(&doorSurface);doors.wire(&doorSurface);auto door=doorRow.findChild<QToolButton *>("openGateToolbarDoor_source");
    do { const bool passed=(door && doorRow.findChildren<QToolButton *>().size()==2); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);if (door) door->click();settle();doors.wire(&doorSurface);
    do { const bool passed=(doorShown && doorCalls==1 && door->isChecked()); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    // Added controls compress before entering the native project title's area.
    QWidget crowded;crowded.setAttribute(Qt::WA_DontShowOnScreen);crowded.resize(900,60);
    QWidget nativeRow(&crowded);nativeRow.setObjectName("frameEditTopBarLeftFixture");nativeRow.setGeometry(0,0,900,40);
    auto nativeLayout=new QHBoxLayout(&nativeRow);nativeLayout->setContentsMargins(0,0,0,0);
    auto nativeButton=new QPushButton("Native",&nativeRow);nativeButton->setFixedWidth(260);nativeLayout->addWidget(nativeButton);
    auto title=new QPushButton("Project title",&crowded);title->setObjectName("btnProjectsMainBarProjectList");title->setGeometry(650,0,150,30);
    Unbound::ToolbarDoors responsive(nullptr);
    for(const auto &id:QStringList{"Timeline","SourceMonitor","ProgramMonitor","page-navigation"})responsive.add({id,id,[]{return true;},[]{}});
    crowded.show();nativeRow.show();title->show();responsive.wire(&crowded);settle();
    auto owned=nativeRow.findChild<QWidget *>("openGateToolbarDoors");
    auto limits=owned->findChild<QWidget *>("openGateDoorControls");
    do { const bool passed=(owned->sizeHint().width()==0 && owned->minimumSizeHint().width()==0); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    const int spacing=nativeLayout->spacing()<0 ? nativeRow.style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) : nativeLayout->spacing();
    do { const bool passed=(nativeRow.minimumSizeHint().width()<=nativeButton->width()+qMax(32,spacing)); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    do { const bool passed=(owned && limits->maximumWidth()<=378); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    title->move(440,0);QEvent resized(QEvent::Resize);QApplication::sendEvent(&crowded,&resized);settle();
    do { const bool passed=(owned->property("openGateDoorMode")=="icons"); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    title->move(310,0);QApplication::sendEvent(&crowded,&resized);settle();
    do { const bool passed=(owned->property("openGateDoorMode")=="overflow"); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    do { const bool passed=(limits->mapToGlobal(QPoint(limits->width(),0)).x() <= title->mapToGlobal(QPoint()).x()); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    crowded.resize(1600,60);nativeRow.resize(1600,40);title->move(1300,0);QApplication::sendEvent(&crowded,&resized);settle();
    do { const bool passed=(owned->property("openGateDoorMode")=="labels"); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    do { const bool passed=(limits->mapToGlobal(QPoint(limits->width(),0)).x() <= title->mapToGlobal(QPoint()).x()); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    auto overflowButton=owned->findChild<QToolButton *>("openGateToolbarOverflow");
    do { const bool passed=(overflowButton->menu()->actions().size()==4 && nativeButton->width()==260 && title->width()==150); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    QWidget scrollOwner;scrollOwner.setAttribute(Qt::WA_DontShowOnScreen);scrollOwner.resize(300,65);
    auto rowContent=new QWidget;rowContent->setMinimumWidth(900);rowContent->setFixedHeight(30);
    auto last=new QPushButton("Last",rowContent);auto contentLayout=new QHBoxLayout(rowContent);contentLayout->addStretch();contentLayout->addWidget(last);
    Unbound::PanelOverflow overflow;auto scroll=qobject_cast<QScrollArea *>(overflow.wrap(rowContent,&scrollOwner,true));
    (new QVBoxLayout(&scrollOwner))->addWidget(scroll);scrollOwner.show();settle();
    do { const bool passed=(scroll->horizontalScrollBar()->maximum()>0 && scroll->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOff && scrollOwner.width()==300); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    scroll->horizontalScrollBar()->setValue(scroll->horizontalScrollBar()->maximum());settle();
    do { const bool passed=(scroll->viewport()->rect().contains(last->mapTo(scroll->viewport(),last->rect().center()))); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    int clicked=0;Unbound::setNativePin(&first,false,[](bool) {});
    ok &= Unbound::setNativeHeaderMenu(&first,true,[&] { ++clicked; });
    const QString longTitle="A long project title that should elide by width while controls remain stationary";
    first.setWindowTitle(longTitle);Unbound::setNativePin(&first,false,[](bool) {});
    do { const bool passed=(Unbound::nativeTitleToolTipForTest(&first)==longTitle); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    auto button=Unbound::nativeHeaderMenuFrameForTest(&first);auto traffic=Unbound::nativeTrafficFramesForTest(&first);
    do { const bool passed=(!button.isEmpty() && traffic.size()==4); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    do { const bool passed=(button.height()<=18 && button.width()<=70); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    for (const auto &frame:traffic) do { const bool passed=(!frame.intersects(button)); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    Unbound::clickNativeHeaderMenuForTest(&first);do { const bool passed=(clicked==1); if(!passed)std::fprintf(stderr,"FAIL header line %d\n",__LINE__); ok &= passed; } while(false);
    std::printf("workspace headers %s\n",ok ? "PASS" : "FAIL");return ok ? 0 : 1;
}
