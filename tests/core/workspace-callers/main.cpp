// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QMenu>
#include <QTabWidget>
#include <cstdio>
#include "../../../src/core/workspacepanelcallers.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    int failures=0;
    auto check=[&](bool value,const char *message){if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}};
    QWidget floating;floating.setAttribute(Qt::WA_DontShowOnScreen);floating.setGeometry(120,150,420,350);floating.show();
    QWidget group;group.setAttribute(Qt::WA_DontShowOnScreen);auto tabs=new QTabWidget(&group);auto pool=new QWidget;auto index=new QWidget;
    tabs->addTab(pool,"Media Pool");tabs->addTab(index,"Index");group.show();
    QString page="edit";int pageQueries=0,calls=0;bool available=true;
    Unbound::WorkspacePanelCallers callers;
    callers.currentPage=[&]{++pageQueries;return page;};
    callers.add({"mixer","Mixer","edit",[&]{return available;},[&]{return floating.isVisible();},[&]{floating.show();},[&](bool shown){++calls;floating.setVisible(shown);}});
    callers.add({"index","Index","edit",[]{return true;},[&]{return tabs->currentWidget()==index;},[&]{tabs->setCurrentWidget(index);},[&](bool shown){++calls;if(shown)tabs->setCurrentWidget(index);}});
    auto action=[&](QMenu &menu,const QString &label){for(auto item:menu.actions())if(item->text()==label)return item;return static_cast<QAction *>(nullptr);};
    QMenu root;callers.wire(&root);check(pageQueries==1,"one context lookup per inventory refresh");
    auto host=floating.window();const auto frame=floating.geometry();
    {QMenu menu;callers.populateFlat(&menu);auto item=action(menu,"Mixer");check(item&&item->isChecked(),"visible floating panel has a checked door");if(item)item->trigger();QApplication::processEvents();}
    check(!floating.isVisible()&&calls==1,"door hides retained floating panel once");
    {QMenu menu;callers.populateFlat(&menu);auto item=action(menu,"Mixer");check(item&&!item->isChecked(),"hidden floating panel has an unchecked door");if(item)item->trigger();QApplication::processEvents();}
    check(floating.isVisible()&&floating.window()==host&&floating.geometry()==frame&&calls==2,"recall keeps the same floating host and geometry");
    const auto owner=index->parentWidget();const auto groupFrame=group.geometry();
    {QMenu menu;callers.populateFlat(&menu);auto item=action(menu,"Index");check(item&&!item->isChecked(),"inactive tab is not presented");if(item)item->trigger();QApplication::processEvents();}
    check(tabs->currentWidget()==index&&index->parentWidget()==owner&&group.geometry()==groupFrame,"inactive tab recall selects it without placement changes");
    {QMenu menu;callers.populateFlat(&menu);auto item=action(menu,"Mixer");if(item)item->trigger();page="color";QApplication::processEvents();}
    check(floating.isVisible()&&calls==3,"queued caller cannot cross a page change");
    page="edit";available=false;
    {QMenu menu;callers.populateFlat(&menu);auto item=action(menu,"Mixer");check(item&&!item->isEnabled(),"unavailable panel cannot be invoked");}
    std::printf("shared workspace caller failures=%d\n",failures);return failures?1:0;
}
