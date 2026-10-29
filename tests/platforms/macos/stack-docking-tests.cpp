// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <cstdio>
#include "dockscopetargets.h"
#include "panellayoutsurface.h"
#include "panellayoutsnapshot.h"
#include "nativearrangement.h"
#include "panelgroup.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");int failures=0;
 auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
 using namespace Unbound;
 QWidget host;host.setAttribute(Qt::WA_DontShowOnScreen);host.resize(1000,800);auto layout=new QVBoxLayout(&host);
 QHash<QString,QWidget *> cells;for(const auto &id:{"source","program","timeline"}){auto w=new QWidget(&host);w->setProperty("groupPanel",id);w->setWindowTitle(id);w->setMinimumSize(100,100);cells[id]=w;}
 const auto column=splitGroupTree({{"panel","source"}},{{"panel","program"}},Qt::Vertical);
 const auto tree=splitGroupTree(column,{{"panel","timeline"}},Qt::Horizontal);
 PanelLayoutCallbacks callbacks;callbacks.leaf=[&](QString id){return cells.value(id);};callbacks.title=[](QString id){return id=="source"?QString("Source Monitor"):id=="program"?QString("Program Monitor"):id=="mixer"?QString("Mixer"):QString("Timeline");};
 auto root=buildPanelLayoutSurface(tree,&host,&host,callbacks);layout->addWidget(root);for(auto cell:cells)cell->show();root->show();host.show();app.processEvents();layout->activate();app.processEvents();
 const auto leaves=cells.values();auto source=cells["source"];auto stack=source->parentWidget();
 const QRect sf(source->mapToGlobal(QPoint()),source->size()),stackFrame(stack->mapToGlobal(QPoint()),stack->size()),outer(root->mapToGlobal(QPoint()),root->size());
 for(const auto &point:{QPoint(sf.left()+2,sf.top()+sf.height()/4),QPoint(sf.right()-2,sf.top()+sf.height()/4),QPoint(sf.center().x(),sf.top()+2),QPoint(sf.center().x(),sf.bottom()-2)}) {
  const auto hit=selectDockScope(leaves,&host,point);check(hit.surface==source&&hit.frame==sf,"each individual panel edge keeps its exact panel bounds");
 }
 const QList<QPoint> points{{stackFrame.left()+31,stackFrame.center().y()},{stackFrame.right()-31,stackFrame.center().y()},{stackFrame.center().x(),stackFrame.top()+31},{stackFrame.center().x(),stackFrame.bottom()-31}};
 const QList<DropPlacement> placements{DropPlacement::Left,DropPlacement::Right,DropPlacement::Above,DropPlacement::Below};
 for(int i=0;i<points.size();++i){const auto hit=selectDockScope(leaves,&host,points[i]);if(hit.surface!=stack)std::fprintf(stderr,"Stack hit diagnostic: edge=%d point=%d,%d stack=%d,%d %dx%d selected=%s section=%s\n",i,points[i].x(),points[i].y(),stackFrame.x(),stackFrame.y(),stackFrame.width(),stackFrame.height(),hit.surface?qPrintable(hit.surface->objectName()):"none",qPrintable(dockSectionKey(hit.surface)));check(hit.surface==stack&&hit.frame==stackFrame&&hit.drop.placement==placements[i],"each stack edge is reachable independently of its individual panels");check(stackFrame.contains(hit.drop.preview),"stack preview stays within the selected stack");}
 const auto full=selectDockScope(leaves,&host,QPoint(outer.left()+2,outer.center().y()));check(full.surface==root,"outer stack scope is independently reachable");
 int neutralPoints=0;for(int x=outer.left();x<qMin(outer.right(),outer.left()+80);++x)neutralPoints+=!selectDockScope(leaves,&host,QPoint(x,outer.center().y())).surface;
 check(neutralPoints>=4,"neutral space between nearby stack targets remains reachable on narrow layouts");
 const auto key=groupSectionTarget(column);const QJsonObject added{{"panel","mixer"}};
 const auto besideStack=insertGroupTree(tree,added,key,Qt::Horizontal,true);
 const auto besidePanel=insertGroupTree(tree,added,"source",Qt::Horizontal,true);
 check(besideStack["children"].toArray()[0].toObject()["children"].toArray()[1].toObject()==column,"stack insertion preserves the complete vertical pair as one subtree");
 check(besidePanel["children"].toArray()[0].toObject()["children"].toArray()[1].toObject()==QJsonObject{{"panel","program"}},"individual insertion leaves the sibling panel in place");
 check(groupTreeHasTarget(tree,key)&&!groupTreeHasTarget(tree,groupSectionTarget(splitGroupTree(added,{{"panel","source"}},Qt::Vertical))),"stale or unrelated stack targets are rejected");
 check(!selectDockScope(leaves,&host,host.mapToGlobal(QPoint(-1,-1))).surface,"desktop space never becomes a stack target");
 check(panelLayoutTitle(column,callbacks.title)=="Source Monitor + Program Monitor","composite tab names use the actual panel names");
 check(panelLayoutTitle(splitGroupTree({{"panel","program"}},{{"panel","mixer"}},Qt::Horizontal),callbacks.title)=="Program Monitor + Mixer","reported Program/Mixer section uses actual panel names instead of Panel");
 auto tabTree=splitGroupTree(column,{{"panel","timeline"}},Qt::Orientation(0));
 auto tabs=buildPanelLayoutSurface(tabTree,&host,&host,callbacks);
 auto tabWidget=qobject_cast<QTabWidget *>(tabs);check(tabWidget&&tabWidget->tabText(0)=="Source Monitor + Program Monitor"&&tabWidget->tabText(1)=="Timeline","nested tab construction never calls a valid section Panel");
 // Native mounting uses the same subtree target and keeps exact native owners.
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1000,700);auto mainLayout=new QVBoxLayout(&main);auto page=new QWidget(&main);page->setObjectName("WStackPage_Conform");mainLayout->addWidget(page);auto original=new QHBoxLayout(page);
 QList<QWidget *> panes;for(const auto &id:{"source","program","mixer"}){auto pane=new QWidget(page);pane->setObjectName(id);auto body=new QVBoxLayout(pane);body->addWidget(new QLabel(id,pane));original->addWidget(pane);panes.append(pane);}main.show();app.processEvents();
 ManagedPanelSession a,b,c;check(a.undock(panes[0],page,"Source Monitor").isEmpty()&&b.undock(panes[1],page,"Program Monitor").isEmpty()&&c.undock(panes[2],page,"Mixer").isEmpty(),"native stack test acquires original panel content");
 NativeArrangement arrangement;check(arrangement.initialize(page,{{"source",&a},{"program",&b}},column).isEmpty(),"native stack mounts through the existing owner");
 check(arrangement.add({{"mixer",&c}},added,key,Qt::Horizontal,true).isEmpty(),"native stack accepts a whole-section split");
 check(arrangement.snapshot()["children"].toArray()[1].toObject()["orientation"]=="vertical","native section split preserves the original vertical stack");
 check(arrangement.restore(false)&&a.dock().isEmpty()&&b.dock().isEmpty()&&c.dock().isEmpty(),"native stack docking remains reversible");
 check(panes[0]->parentWidget()==page&&panes[1]->parentWidget()==page&&panes[2]->parentWidget()==page&&page->layout()==original,"Native return restores original owners and layout");
 check(a.undock(panes[0],page,"Source Monitor").isEmpty()&&b.undock(panes[1],page,"Program Monitor").isEmpty()&&c.undock(panes[2],page,"Mixer").isEmpty(),"floating stack reacquires the original panels");
 a.window()->setObjectName("unboundFloating_source");b.window()->setObjectName("unboundFloating_program");c.window()->setObjectName("unboundFloating_mixer");
 PanelGroup group(nullptr);check(group.join(&a,&b,Qt::Vertical).isEmpty(),"floating stack uses the existing group session");
 check(group.add(&c,Qt::Horizontal,true,key).isEmpty(),"floating stack accepts the same whole-section target");
 check(group.snapshot()["tree"].toObject()["children"].toArray()[1].toObject()["orientation"]=="vertical","floating section insertion preserves the vertical pair");
 group.dissolve(true);check(panes[0]->parentWidget()==page&&panes[1]->parentWidget()==page&&panes[2]->parentWidget()==page,"floating stack Native return restores original owners");
 if(!failures)std::puts("PASS: individual/all stack edges, nested section insertion, accurate titles, stale-target rejection and native ownership recovery");return failures?1:0;
}
