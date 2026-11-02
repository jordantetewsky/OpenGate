// SPDX-License-Identifier: MIT
#pragma once
#include <QTemporaryDir>
#include <QSaveFile>
#include <QLineEdit>
#include "nativepresetfixture.h"
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcommandcatalog.h"

namespace OpenGateTest {
template<class Check> void timelinePointerChecks(Check check) {
    QTemporaryDir temporary;
    const auto previous=qgetenv("OPENGATE_PRESET_DIRECTORY");
    qputenv("OPENGATE_PRESET_DIRECTORY",temporary.path().toUtf8());
    auto save=[&](const char *name,const QByteArray &bytes) {
        QSaveFile file(temporary.path()+"/"+name);file.open(QIODevice::WriteOnly);file.write(bytes);file.commit();
    };
    save("config.user.xml","<Config><CustomKeyboardPreset>Active</CustomKeyboardPreset></Config>");
    save("keyboard.preset.xml",nativePresetFixture({{"controlGoToMousePointer",{QKeySequence(Qt::Key_Tab)}}}));
    QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1100,800);
    QWidget edit(&main);edit.setObjectName("WStackPage_Conform");edit.resize(main.size());edit.show();
    TimelineViewportFixture fixture(&edit);fixture.timeline.show();main.show();
    QApplication::setActiveWindow(&main);fixture.view->setFocus();
    Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",2.},{"timeline_header_width",160.}});
    check(scale.apply(&fixture.timeline),"pointer-command fixture uses the assembled content adapter");TimelineViewportFixture::settle();
    auto viewport=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    if(!viewport)return;
    Unbound::PanelCommandCatalog commands(&main,[]{return true;});
    QShortcut native(QKeySequence(Qt::Key_Tab),&main);native.setContext(Qt::ApplicationShortcut);
    int calls=0;QPoint received;
    QObject::connect(&native,&QShortcut::activated,[&] {++calls;received=fixture.view->viewport()->mapFromGlobal(QCursor::pos());});
    for(double factor:{.7,.73,.85,1.,1.25,1.47,2.,2.13,2.63,3.}) {
        scale.setValue(factor);scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto layer=viewport->presentation();const auto physical=QCursor::pos();
        const QPoint target(qRound((viewport->headerWidth()+40)*factor),qRound(65*factor));
        fixture.timeline.move(fixture.timeline.pos()+physical-layer->mapToGlobal(target));TimelineViewportFixture::settle();
        QApplication::setActiveWindow(&main);fixture.view->setFocus();
        const int previousCalls=calls;
        QShortcutEvent event(QKeySequence(Qt::Key_Tab),native.id(),false);QApplication::sendEvent(&native,&event);
        if(calls!=previousCalls+1 || (received-QPoint(300,65)).manhattanLength()>=3)
            std::fprintf(stderr,"pointer diagnostic factor %.2f calls %d/%d received %d,%d page %s focus %s physical-local %d,%d owner-count %d context %d block %s\n",
                factor,calls,previousCalls,received.x(),received.y(),qPrintable(Unbound::currentWorkspacePage(Unbound::resolveMainWindow())),
                QApplication::focusWidget()?QApplication::focusWidget()->metaObject()->className():"none",
                layer->mapFromGlobal(physical).x(),layer->mapFromGlobal(physical).y(),commands.nativeShortcutOwners(QKeySequence(Qt::Key_Tab)).size(),
                commands.context(),qPrintable(Unbound::PanelCommandContext::blocked()));
        check(calls==previousCalls+1 && (received-QPoint(300,65)).manhattanLength()<3,
              "native Tab Go to Mouse Pointer handler runs once with scaled and narrowed displayed coordinates");
        TimelineViewportFixture::settle();
        check((fixture.view->viewport()->mapFromGlobal(physical)-QPoint(300,65)).manhattanLength()<3 && QCursor::pos()==physical,
              "native pointer coordinates remain valid through queued command work without moving the OS pointer");
    }
    QWidget pool(&edit);pool.setObjectName("frameEditPageMediaPool");pool.setFocusPolicy(Qt::StrongFocus);pool.show();pool.setFocus();
    QShortcut timelineKey(QKeySequence(Qt::Key_W),fixture.view),poolKey(QKeySequence(Qt::Key_W),&pool);
    timelineKey.setContext(Qt::WidgetWithChildrenShortcut);poolKey.setContext(Qt::WidgetWithChildrenShortcut);
    int timelineCalls=0,poolCalls=0;
    QObject::connect(&timelineKey,&QShortcut::activated,[&]{++timelineCalls;});
    QObject::connect(&poolKey,&QShortcut::activated,[&]{++poolCalls;});
    auto pressW=[&](QWidget *target) {
        target->setFocus();TimelineViewportFixture::settle();
        QKeyEvent over(QEvent::ShortcutOverride,Qt::Key_W,Qt::NoModifier,"w");QApplication::sendEvent(target,&over);
        QKeyEvent down(QEvent::KeyPress,Qt::Key_W,Qt::NoModifier,"w");QApplication::sendEvent(target,&down);
    };
    pressW(fixture.view);check(timelineCalls==1 && poolCalls==0,"ordinary native key command reaches the scaled Timeline once");
    pressW(&pool);check(timelineCalls==1 && poolCalls==1,"same native key remains available in the other focused panel");
    QLineEdit editor(fixture.view->viewport());editor.show();pressW(&editor);
    check(editor.text()=="w" && timelineCalls==1 && poolCalls==1,"native shortcut adaptation preserves inline text editing at 300 percent");
    pool.setFocus();
    const auto before=fixture.view->pos();const auto previousCalls=calls;
    QShortcutEvent event(QKeySequence(Qt::Key_Tab),native.id(),false);QApplication::sendEvent(&native,&event);
    check(calls==previousCalls+1 && fixture.view->pos()==before,
          "the same native shortcut outside Timeline retains its ordinary handler and coordinates");
    auto disposable=new Unbound::PanelCommands(&main);disposable->context=[]{return true;};
    disposable->nativeShortcutDispatch=[&](QObject *,QShortcutEvent *) {delete disposable;disposable=nullptr;return false;};
    QShortcutEvent retiring(QKeySequence(Qt::Key_Tab),native.id(),false);QApplication::sendEvent(&native,&retiring);
    check(!disposable,"native shortcut dispatch safely tolerates its adapter owner retiring during handling");
    scale.suspend();
    if(previous.isNull())qunsetenv("OPENGATE_PRESET_DIRECTORY");else qputenv("OPENGATE_PRESET_DIRECTORY",previous);
}
}
