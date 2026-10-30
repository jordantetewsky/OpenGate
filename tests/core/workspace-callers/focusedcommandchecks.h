// SPDX-License-Identifier: MIT
#pragma once
#include <QLineEdit>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include "panelcommands.h"
namespace OpenGateTest {
// Called by the existing command harness; no second app/settings/log harness.
template<class Check> void focusedCommandChecks(QWidget &main,Check check) {
    QWidget timeline(&main),pool(&main),child(&timeline);
    timeline.setObjectName("frameConformTimelines"); pool.setProperty("groupPanel","frameEditPageMediaPool");
    for (auto widget:{&timeline,&pool,&child}) { widget->setFocusPolicy(Qt::StrongFocus);widget->show(); }
    QApplication::setActiveWindow(&main);
    Unbound::PanelCommands scoped(nullptr);scoped.context=[] {return true;};
    int timelineCalls=0,poolCalls=0;
    scoped.add({"timeline","Timeline command",[]{return true;},[&]{++timelineCalls;},"frameConformTimelines"});
    scoped.add({"pool","Media Pool command",[]{return true;},[&]{++poolCalls;},"frameEditPageMediaPool"});
    check(scoped.save({{"timeline","F19"},{"pool","F19"}}).isEmpty(),"same key allowed in disjoint panels");
    child.setFocus();QApplication::processEvents();
    check(scoped.focusedTarget().panel=="frameConformTimelines","native focused descendant resolves actual Timeline");
    auto press=[&](QWidget *receiver,bool drain=true) {
        QKeyEvent override(QEvent::ShortcutOverride,Qt::Key_F19,Qt::NoModifier);
        QApplication::sendEvent(receiver,&override);
        QKeyEvent key(QEvent::KeyPress,Qt::Key_F19,Qt::NoModifier);
        QApplication::sendEvent(receiver,&key);
        if(drain) for(int i=0;i<4;++i)QApplication::processEvents();
    };
    press(&child);check(timelineCalls==1&&poolCalls==0,"Timeline context dispatches once");
    pool.setFocus();QApplication::processEvents();press(&pool);
    check(timelineCalls==1&&poolCalls==1,"grouped Media Pool context reuses same key once");
    QLineEdit rename(&pool);rename.show();rename.setFocus();QApplication::processEvents();press(&rename);
    check(timelineCalls==1&&poolCalls==1,"rename editor retains modified or function keys");
    check(!scoped.invoke("pool","resolve-workflow-shortcut"),"workflow route also yields to text editing");
    child.setFocus();QApplication::processEvents();press(&child,false);
    pool.setFocus();for(int i=0;i<4;++i)QApplication::processEvents();
    check(timelineCalls==1&&poolCalls==1,"queued key cannot retarget after focus changes");
    scoped.add({"overlap","Other Timeline command",[]{return true;},[]{},"frameConformTimelines"});
    check(!scoped.validate({{"timeline","F19"},{"overlap","F19"}}).isEmpty(),"overlapping panel scopes reject duplicate key");
    scoped.add({"global","Global command",[]{return true;},[]{}});
    check(!scoped.validate({{"timeline","F19"},{"global","F19"}}).isEmpty(),"global binding overlaps panel scope");
    QGraphicsScene scene;QGraphicsView graphics(&scene,&timeline);
    auto text=scene.addText("Rename");text->setTextInteractionFlags(Qt::TextEditorInteraction);
    graphics.show();graphics.setFocus();scene.setFocusItem(text);QApplication::processEvents();
    check(Unbound::textEditor(&graphics),"editable native graphics text is protected");
    main.setFocus();QApplication::processEvents();
}
}
