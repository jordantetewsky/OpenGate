// SPDX-License-Identifier: MIT
#pragma once
// Exercise Mixer through the same arranged panel doors and controls as its
// neighbors. Native-return coverage remains in the assembled entry-flow test.
template<class Check,class Settle>
void checkMixerPanel(PanelControls &controls,QWidget &main,Unbound::NativeWorkspaceBinding *binding,
 QWidget *timeline,QWidget *title,QWidget *content,QAbstractButton *fader,int &nativeCalls,bool displayed,Check check,Settle settle) {
 auto currentCell=[&]() -> QWidget * {
  for(auto parent=title->parentWidget();parent;parent=parent->parentWidget())
   if(parent->objectName()=="unboundGroupCell_frameEditAudioMeter")return parent;
  return nullptr;
 };
 QWidget *cell=currentCell();auto timelineCell=main.findChild<QWidget *>("unboundGroupCell_frameConformTimelines");
 check(binding->contains("frameEditAudioMeter")&&cell&&cell->isVisible(),"default layout gives Mixer its own visible panel leaf");
 check(cell&&timelineCell&&cell->parentWidget()==timelineCell->parentWidget(),"Mixer starts beside Timeline in the same split");
 auto timelineRow=timelineCell?timelineCell->parentWidget():nullptr;
 auto center=timelineRow?qobject_cast<QSplitter *>(timelineRow->parentWidget()):nullptr;
 if(center&&center->orientation()==Qt::Vertical) {
  const auto sizes=center->sizes();const int total=sizes.value(0)+sizes.value(1);
  check(total>0&&sizes.value(1)*100/total>=35&&sizes.value(1)*100/total<=45,
   "default preset presents Timeline and Mixer at their intended 40 percent share of the center height");
 } else check(false,"default preset has its viewer row above the Timeline and Mixer row");
 auto header=cell?cell->findChild<QLabel *>("unboundGroupHeader"):nullptr;
 check(title->isHidden(),"managed Mixer header suppresses the redundant native name");
 check(header&&header->text()=="Mixer"&&header->isVisible(),"Mixer starts with the standard readable panel header");
 if(displayed&&header) {
  Unbound::activateNativeWindowForTest(&main);settle(150);const auto cursor=QCursor::pos();
  QCursor::setPos(header->mapToGlobal(QPoint(25,12)));settle(100);
  check(controls.invokeWorkflowCommand("panel.expand.hovered"),"pointer maximize dispatches from the actual managed Mixer header");settle(300);
  auto expanded=title->window();check(expanded!=&main&&timeline->window()==&main,"pointer maximize expands only Mixer");
  Unbound::activateNativeWindowForTest(expanded);controls.invokeWorkflowCommand("panel.expand.hovered");settle(300);
  check(title->window()==&main&&binding->contains("frameEditAudioMeter"),"pointer restore returns Mixer to its retained neighbor placement");QCursor::setPos(cursor);
  cell=currentCell();
 }
 auto button=cell?cell->findChild<QToolButton *>("unboundDetachPanel"):nullptr;
 check(button&&button->isVisible(),"Mixer exposes the same placement control as neighboring panels");
 if(button)button->click();settle(350);
 check(!title->isHidden(),"detaching Mixer restores its native name");
 auto host=title->window();check(host!=&main&&timeline->window()==&main,"Mixer detaches independently from Timeline");
 host->setGeometry(180,140,420,360);settle(150);const auto frame=host->geometry();
 QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.toggle.frameEditAudioMeter");settle(200);
 check(!title->isVisible(),"Mixer toggle hides the retained panel");
 auto callers=component<Unbound::PanelCallerMenu>(&controls);
 auto recall=[&] {QMenu menu;callers->populateFlat(&menu);
  for(auto action:menu.actions())if(action->text()=="Mixer")action->trigger();settle(250);};
 recall();
 check(title->isVisible()&&title->window()==host&&host->geometry()==frame,"Panels recalls Mixer in its retained host and geometry");
 QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameEditAudioMeter");settle(250);
 check(host->geometry()!=frame,"Mixer uses ordinary panel maximize");
 check(content->maximumWidth()==QWIDGETSIZE_MAX&&content->width()>864,"maximized Mixer content fills beyond its embedded width cap");
 check(fader->maximumWidth()==70,"Mixer native track constraints are preserved");fader->click();
 check(nativeCalls==1,"native Mixer control remains callable during expansion");
 QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameEditAudioMeter");settle(250);
 check(title->window()==host&&host->geometry()==frame&&content->maximumWidth()==864,"restore reinstates Mixer geometry and exact native constraint");
 QApplication::setActiveWindow(&main);controls.invokeWorkflowCommand("panel.expand.frameConformTimelines");settle(250);
 controls.invokeWorkflowCommand("panel.expand.frameConformTimelines");settle(250);
 host->close();settle(200);
 check(!title->isVisible(),"closing Mixer after Mixer and Timeline expansion hides its retained host");
 recall();
 check(title->isVisible()&&title->window()==host&&host->geometry()==frame,"recall after both expansions retains Mixer host and geometry");
 auto source=main.findChild<QWidget *>("unboundGroupCell_widgetEditSourcePlayer");
 auto floating=dynamic_cast<Unbound::WorkspaceFloatingHost *>(host);
 check(source&&floating&&floating->attachRequested,"Mixer can dock beside another panel");
 if(source&&floating&&floating->attachRequested)floating->attachRequested(source,Qt::LeftEdge);settle(350);
 cell=currentCell();header=cell?cell->findChild<QLabel *>("unboundGroupHeader"):nullptr;
 check(binding->contains("frameEditAudioMeter")&&header&&header->isVisible(),"repositioned Mixer retains the standard panel header");
 if(header) {
  const QPoint p(25,12);QMouseEvent down(QEvent::MouseButtonPress,p,header->mapToGlobal(p),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(header,&down);
  QMouseEvent move(QEvent::MouseMove,p+QPoint(90,0),header->mapToGlobal(p+QPoint(90,0)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(header,&move);
  QMouseEvent up(QEvent::MouseButtonRelease,p+QPoint(90,0),header->mapToGlobal(p+QPoint(90,0)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(header,&up);settle(350);
 }
 check(title->window()!=&main&&!binding->contains("frameEditAudioMeter"),"standard Mixer title drag detaches only Mixer");
 // Return it beside Timeline, preserving the assembled tree for later checks.
 floating=dynamic_cast<Unbound::WorkspaceFloatingHost *>(title->window());timelineCell=main.findChild<QWidget *>("unboundGroupCell_frameConformTimelines");
 if(floating&&timelineCell&&floating->attachRequested)floating->attachRequested(timelineCell,Qt::RightEdge);settle(350);
 check(title->window()==&main&&binding->contains("frameEditAudioMeter"),"Mixer returns beside Timeline as an independent panel");
}
