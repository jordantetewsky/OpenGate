// SPDX-License-Identifier: MIT
#pragma once
#include <QListWidget>
#include "nativeclipviewfixture.h"
#include "../../../src/core/panelcontrols.h"

inline QJsonObject navigationPlacement(QJsonObject node) {
    node.remove("selected"); // Revealing a destination intentionally selects its tab.
    auto children=node["children"].toArray();
    for(int i=0;i<children.size();++i)children[i]=navigationPlacement(children[i].toObject());
    if(node.contains("children"))node["children"]=children;
    return node;
}

// Reuse the assembled workspace and native controls; no second startup harness.
template<class Check,class Settle>
void checkNativePanelNavigation(PanelControls &controls,QWidget &main,QWidget *page,
    Unbound::NativeWorkspaceBinding *binding,QListWidget *clips,
    const QHash<QString,QWidget *> &nativePanels,Check check,Settle settle) {
  auto modeButton=new QPushButton(nativePanels["widgetEditSourcePlayer"]);modeButton->setObjectName("btnSourceClipMode");
  NativeClipViewFixture *nativeClips=nullptr;
  if(qEnvironmentVariableIntValue("OPENGATE_TEST_NATIVE_CLIP_VIEW")) {
   nativeClips=new NativeClipViewFixture(modeButton);clips->parentWidget()->layout()->addWidget(nativeClips);clips->hide();settle(100);
  }
  auto viewport=nativeClips?nativeClips->viewport():clips->viewport();
  auto dispatch=[&](QPoint point,Qt::MouseButton button=Qt::LeftButton){
   QMouseEvent event(QEvent::MouseButtonDblClick,point,viewport->mapToGlobal(point),button,button,Qt::NoModifier);
   QApplication::sendEvent(viewport,&event);
   QMouseEvent release(QEvent::MouseButtonRelease,point,viewport->mapToGlobal(point),button,Qt::NoButton,Qt::NoModifier);
   QApplication::sendEvent(viewport,&release);settle(100);
  };
  auto cell=main.findChild<QWidget *>("unboundGroupCell_widgetEditSourcePlayer");
  QPointer<QWidget> retained=cell;const QJsonValue tree=binding->saved()["tree"];
  binding->setVisible("widgetEditSourcePlayer",false);settle(100);
  dispatch(QPoint(-2,-2));check(!binding->selected("widgetEditSourcePlayer"),"blank Media Pool input does not recall Source");
  auto point=nativeClips?QPoint(20,20):clips->visualItemRect(clips->item(0)).center();
  dispatch(point,Qt::RightButton);check(!binding->selected("widgetEditSourcePlayer"),"right double-click does not recall Source");
  if(nativeClips){dispatch(QPoint(110,65));check(!binding->selected("widgetEditSourcePlayer"),"custom-view blank input does not recall Source");
   modeButton->click();settle(50);check(!binding->selected("widgetEditSourcePlayer"),"background Source activation after release does not recall Source");}
  dispatch(point);
  check(binding->selected("widgetEditSourcePlayer")&&retained==main.findChild<QWidget *>("unboundGroupCell_widgetEditSourcePlayer"),"clip double-click recalls the existing hidden Source cell");
  check(tree==binding->saved()["tree"],"Source recall preserves the managed tree");
  auto placement=controls.findChild<QPushButton *>("undock_widgetEditSourcePlayer");if(placement)placement->click();settle(300);
  Unbound::ManagedPanelSession *source=nullptr;
  for(auto child:controls.children())if(auto session=dynamic_cast<Unbound::ManagedPanelSession *>(child))
   if(session->active()&&session->window()&&session->window()->objectName()=="unboundFloating_widgetEditSourcePlayer")source=session;
  check(source,"Source recall fixture finds retained floating session");
  if(source){auto host=source->window();auto geometry=host->geometry();source->setHiddenByUser(true);settle(100);dispatch(point);
   check(source->window()==host&&host->isVisible()&&host->geometry()==geometry,"clip double-click recalls floating Source without replacing or moving it");}
  check(binding->dockSelection(page,"widgetEditSourcePlayer",nullptr,"widgetEditTimelinePlayer",Qt::Orientation(0),false).isEmpty(),"fixture places Source in a retained viewer tab group");settle(200);
  binding->setVisible("widgetEditTimelinePlayer",true);settle(100);
  auto tabCell=main.findChild<QWidget *>("unboundGroupCell_widgetEditSourcePlayer");QPointer<QWidget> tabOwner=tabCell?tabCell->parentWidget():nullptr;
  auto program=nativePanels["widgetEditTimelinePlayer"];auto programFrame=program->geometry();
  check(!binding->selected("widgetEditSourcePlayer"),"Source starts behind Program tab");dispatch(point);
  check(binding->selected("widgetEditSourcePlayer")&&tabCell&&tabCell->parentWidget()==tabOwner,"clip double-click selects the existing Source tab");
  check(program->geometry()==programFrame,"tab recall retains the Program viewer geometry");
  const auto recalledPlan=binding->saved();dispatch(point);
  check(binding->saved()==recalledPlan,"repeated double-click on already selected Source does not change layout state");
  int matches=0,finds=0;
  auto match=new QAction("Match Frame",&main);match->setObjectName("timelineMatchFrame");
  QObject::connect(match,&QAction::triggered,&main,[&]{++matches;});main.addAction(match);
  auto find=new QAction("Find in Media Pool",&main);find->setObjectName("fixtureNativeFind");
  QObject::connect(find,&QAction::triggered,&main,[&]{++finds;});main.addAction(find);
  binding->setVisible("widgetEditSourcePlayer",false);settle(50);match->trigger();settle(100);
  check(matches==1&&binding->selected("widgetEditSourcePlayer"),"native Match Frame executes once and recalls hidden Source in place");
  binding->setVisible("frameEditPageMediaPool",false);settle(50);const QJsonValue findTree=binding->saved()["tree"];
  find->trigger();settle(100);
  check(finds==1,"native Find executes exactly once");
  check(binding->selected("frameEditPageMediaPool"),"native Find recalls and selects Media Pool");
  check(navigationPlacement(binding->saved()["tree"].toObject())==navigationPlacement(findTree.toObject()),"native Find retains placement while selecting the destination tab");
  binding->setVisible("frameEditPageMediaPool",false);settle(50);
  int nativeFinds=0;
  QShortcut shortcut(QKeySequence("Alt+G"),&main);
  QObject::connect(&shortcut,&QShortcut::activated,&main,[&]{++nativeFinds;});
  auto sendShortcut=[&](bool ambiguous=false) {
   QShortcutEvent event(shortcut.key(),shortcut.id(),ambiguous);QApplication::sendEvent(&shortcut,&event);settle(100);
  };
  sendShortcut();check(nativeFinds==1&&binding->selected("frameEditPageMediaPool"),"remapped native Find shortcut executes once and recalls Media Pool");
  binding->setVisible("frameEditPageMediaPool",false);settle(50);
  sendShortcut(true);check(nativeFinds==1&&!binding->selected("frameEditPageMediaPool"),"ambiguous native shortcut cannot recall Media Pool");
  shortcut.setKey(QKeySequence("R"));sendShortcut();
  check(nativeFinds==2&&!binding->selected("frameEditPageMediaPool"),"changing a wired shortcut to an unrelated assignment removes Find association");
  auto showOnly=new QAction("Media Pool",&main);main.addAction(showOnly);showOnly->trigger();settle(50);
  check(!binding->selected("frameEditPageMediaPool"),"Show Media Pool is not treated as a Find command");
  binding->setVisible("frameEditPageMediaPool",true);
  main.removeAction(match);main.removeAction(find);main.removeAction(showOnly);delete match;delete find;delete showOnly;
  if(nativeClips){delete nativeClips;clips->show();}delete modeButton;

}
