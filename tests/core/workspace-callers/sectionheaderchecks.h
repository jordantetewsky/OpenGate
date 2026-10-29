// SPDX-License-Identifier: MIT
#pragma once
// Reproduce a Source/Media Pool neighbor section inside the default's tab group.
template<class Check,class Settle>
void checkSectionHeaders(PanelControls &controls,QWidget &main,QWidget *source,Check check,Settle settle) {
 auto detach=controls.findChild<QPushButton *>("undock_widgetEditSourcePlayer");
 if(detach)detach->click();settle(350);
 QWidget *pool=nullptr;
 for(auto cell:main.findChildren<QWidget *>("unboundGroupCell_frameEditPageMediaPool"))if(cell->isVisible())pool=cell;
 auto host=dynamic_cast<Unbound::WorkspaceFloatingHost *>(source->window());
 check(host&&pool&&host->attachRequested,"section fixture finds the Source host and selected Media Pool leaf");
 if(!host||!pool||!host->attachRequested)return;
 host->attachRequested(pool,Qt::BottomEdge);settle(350);
 QWidget *cell=source;
 while(cell&&cell->objectName()!="unboundGroupCell_widgetEditSourcePlayer")cell=cell->parentWidget();
 auto header=cell?cell->findChild<QLabel *>("unboundGroupHeader"):nullptr;
 check(header&&header->isVisible()&&header->text()=="Source Monitor","Source inside a tabbed neighbor section keeps its own readable header");
 QTabWidget *tabs=nullptr;
 for(auto parent=cell?cell->parentWidget():nullptr;parent&&parent!=&main;parent=parent->parentWidget())
  if((tabs=qobject_cast<QTabWidget *>(parent)))break;
 check(tabs&&tabs->currentWidget()!=cell,"the selected tab represents the section rather than Source alone");
 check(tabs&&!tabs->cornerWidget(Qt::TopRightCorner),"a composite section tab does not borrow an arbitrary child's controls");
 auto poolHeader=pool->findChild<QLabel *>("unboundGroupHeader");
 check(poolHeader&&poolHeader->isVisible(),"Media Pool in the same section also retains its own header");
 auto presets=component<Unbound::WorkspaceArrangements>(&controls);
 if(presets&&presets->invoke)presets->invoke("Restore","opengate.resolve-layout.edit");settle(1800);
}
