// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QShowEvent>
#include <QContextMenuEvent>
#include <QThread>
#include <QElapsedTimer>
#include <cstdio>
#include "../../../src/core/clipcolorpreviewcommands.h"
#include "../../../src/core/panelcommandcatalog.h"
#include "../../../src/core/panelcontentscale.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include "../../core/workspace-callers/clipcolorpoolfixture.h"
#include "../../core/workspace-callers/nativepresetfixture.h"
#include "../../core/colors/presetcommandchecks.h"
#include "../../core/workspace-callers/isolatednativeactionchecks.h"
#include <memory>
using namespace OpenGateTest;
class DynamicPalette : public QMenu {
public:
    using QMenu::QMenu;
protected:
    void showEvent(QShowEvent *e) override {
        clear();for(const auto &label:{"Clear Color","Orange","Apricot","Chocolate"})addAction(label);
        for(int i=0;i<80;++i)addAction(QString("Native palette row %1").arg(i));
        QMenu::showEvent(e);
    }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir files;qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    qputenv("OPENGATE_COLOR_DISPLAY_FILE",(files.path()+"/ColorDisplay.json").toUtf8());
    qputenv("OPENGATE_EXPERIMENTAL_MEDIA_POOL_COLORS","1");
    qputenv("OPENGATE_PRESET_DIRECTORY",files.path().toUtf8());qputenv("UNBOUND_SHORTCUTS_FILE",(files.path()+"/shortcuts.json").toUtf8());
    auto write=[&](const char *name,const QByteArray &b){QFile f(files.path()+"/"+name);f.open(QIODevice::WriteOnly);f.write(b);};
    write("config.user.xml","<Config><CustomKeyboardPreset>Active</CustomKeyboardPreset></Config>");
    write("keyboard.preset.xml",nativePresetFixture({
        {"OpenGate - Timeline Clip Color-Preview Red (Temporary)",{QKeySequence(Qt::Key_F6)}},
        {"OpenGate - Media Pool Clip Color-Preview Red (Temporary)",{QKeySequence(Qt::Key_F6)}}}));
    int failures=0;auto check=[&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1300,800);
    QWidget edit(&main);edit.setObjectName("WStackPage_Conform");edit.resize(main.size());edit.show();
    TimelineViewportFixture timeline(&edit);timeline.view->clip->hide();auto clip=new UiTiClipContainer;
    timeline.view->tracksScene->addItem(clip);clip->setSelected(true);
    Unbound::PanelContentScale scale;scale.restoreState({{"timeline_header_width",160.}});scale.apply(&timeline.timeline);
    ClipColorPoolFixture pool(&edit);pool.root.move(920,0);pool.root.show();main.show();
    isolatedNativeActionChecks(&app,check);
    Unbound::PanelCommandCatalog commands(&main,[]{return true;});auto colors=std::make_unique<Unbound::ClipColorPreviewCommands>(&main,&commands);
    TimelineViewportFixture::settle();
    auto popup=[&](QWidget *focus,bool expectPool) {
        QApplication::setActiveWindow(&main);focus->setFocus();TimelineViewportFixture::settle();
        QMenu root;root.setAttribute(Qt::WA_DontShowOnScreen);DynamicPalette palette(&root);palette.setAttribute(Qt::WA_DontShowOnScreen);
        palette.setMaximumHeight(320);
        auto item=root.addAction("Clip Color");item->setMenu(&palette);root.popup(QPoint(300,300));TimelineViewportFixture::settle();
        int nativePaletteCalls=0,nativeRootCalls=0;
        QObject::connect(&palette,&QMenu::triggered,&main,[&](QAction *){++nativePaletteCalls;clip->fill=Qt::green;clip->update();});
        QObject::connect(&root,&QMenu::triggered,&main,[&](QAction *){++nativeRootCalls;});
        const auto point=root.actionGeometry(item).center();QMouseEvent hover(QEvent::MouseMove,point,root.mapToGlobal(point),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(&root,&hover);QElapsedTimer wait;wait.start();
        while(!palette.isVisible() && wait.elapsed()<700){QApplication::processEvents();QThread::msleep(2);}
        TimelineViewportFixture::settle();QAction *red=nullptr;
        for(auto action:palette.actions())if(action->objectName()=="openGateRedClipPreview")red=action;
        check(palette.isVisible() && red && red->isEnabled(),"real unnamed nested submenu gets Red after native Show population and popup focus changes");
        auto swatch=palette.findChild<QWidget *>("openGateMenuColorSwatch");
        check(red && red->text()=="Red" && swatch && swatch->testAttribute(Qt::WA_TransparentForMouseEvents),"Red uses the native row label and a non-interactive right-side swatch");
        bool extraClear=false;for(auto action:palette.actions())extraClear|=action->text().contains("Clear Temporary");
        check(!extraClear,"existing native Clear Color is retained without a second temporary-clear menu row");
        auto scroll=palette.findChild<QScrollBar *>("openGateMenuOverflowScroll");
        check(scroll && scroll->isVisible(),"late-populated overflowing Clip Color submenu exposes a visible scrollbar");
        if(scroll){scroll->setValue(scroll->maximum());TimelineViewportFixture::settle();}
        check(red && palette.rect().contains(palette.actionGeometry(red)),"scrolling the real color submenu makes Red fully reachable before clicking it");
        if(swatch){const auto image=palette.grab().toImage();check(swatch->isVisible() && image.pixelColor((QPointF(swatch->geometry().center())*image.devicePixelRatioF()).toPoint())==QColor(Qt::red),"native Red row shows a red swatch after scrolling");}
        if(red){check(red->property("openGateCommandId").toString()==(expectPool?"clip-color.media-pool.red":"clip-color.timeline.red"),"context menu retains the originating panel command");
            TimelineViewportFixture::click(&palette,swatch?swatch->geometry().center():palette.actionGeometry(red).center());}
        palette.hide();root.hide();TimelineViewportFixture::settle();
        check(nativePaletteCalls==0 && nativeRootCalls==0 && clip->fill==QColor(30,130,180),
            "OpenGate action never reaches native palette/root handlers or changes native model color");
        check(!palette.signalsBlocked() && !root.signalsBlocked(),"native menu signals restore after the OpenGate action");
        if(!expectPool)check(!Unbound::TimelineClipColorBindings::find(timeline.view)->regions().isEmpty(),"Timeline context-menu gesture reaches its actual native clip adapter");
        else check(pool.view.itemDelegateForRow(0)!=nullptr,"Media Pool context-menu gesture reaches its actual model delegate adapter");
        auto native=palette.actions().front();native->trigger();
        check(nativePaletteCalls==1,"ordinary native palette actions still reach their original handler once");
        if(!expectPool)check(Unbound::TimelineClipColorBindings::find(timeline.view)->regions().isEmpty(),"choosing a native palette color clears the selected clip's preview");
        else check(pool.view.itemDelegateForRow(0)==nullptr,"choosing native Clear Color releases selected Media Pool preview paint");
        clip->fill=QColor(30,130,180);clip->update();
    };
    popup(timeline.view,false);
    popup(&pool.view,true);
    auto press=[&](QWidget *focus){QApplication::setActiveWindow(&main);focus->setFocus();TimelineViewportFixture::settle();
        QKeyEvent over(QEvent::ShortcutOverride,Qt::Key_F6,Qt::NoModifier);QApplication::sendEvent(focus,&over);
        QKeyEvent key(QEvent::KeyPress,Qt::Key_F6,Qt::NoModifier);QApplication::sendEvent(focus,&key);TimelineViewportFixture::settle();};
    timeline.view->setFocus();TimelineViewportFixture::settle();commands.invoke("clip-color.timeline.clear","fixture");
    press(timeline.view);check(!Unbound::TimelineClipColorBindings::find(timeline.view)->regions().isEmpty(),"native preset assignment maps Red in Timeline");
    {
        QMenu reused;reused.setAttribute(Qt::WA_DontShowOnScreen);reused.setObjectName("markSetClipColor");
        auto native=reused.addAction("Orange");
        auto showFrom=[&](QWidget *origin) {
            QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(5,5));QApplication::sendEvent(origin,&context);
            reused.popup(QPoint(300,300));TimelineViewportFixture::settle();
        };
        showFrom(timeline.view);reused.hide();TimelineViewportFixture::settle();
        QWidget unsupported(&edit);unsupported.setFocusPolicy(Qt::StrongFocus);unsupported.show();
        showFrom(&unsupported);native->trigger();TimelineViewportFixture::settle();
        check(!Unbound::TimelineClipColorBindings::find(timeline.view)->regions().isEmpty(),"native palette reused outside supported panels cannot clear an old Timeline preview");
        reused.hide();
    }
    pool.view.setFocus();TimelineViewportFixture::settle();commands.invoke("clip-color.media-pool.clear","fixture");
    press(&pool.view);check(pool.view.itemDelegateForRow(0)!=nullptr,"same assigned key maps Red independently in Media Pool");
    check(commands.commandScope("clip-color.timeline.red")!=commands.commandScope("clip-color.media-pool.red"),"mapping scopes are disjoint, not global");
    {
        QApplication::setActiveWindow(&main);pool.view.setFocus();
        auto selection=pool.view.selectionModel();
        for(int row=0;row<2;++row){selection->setCurrentIndex(pool.model.index(row,0),QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);TimelineViewportFixture::settle();commands.invoke("clip-color.media-pool.red","fixture");}
        check(pool.view.itemDelegateForRow(0)&&pool.view.itemDelegateForRow(1),"two Media Pool rows can retain independent preview bindings");
        QAction nativeClear("Native Clear Color",&main);nativeClear.setObjectName("markSetClipColorDefault");nativeClear.setShortcut(QKeySequence(Qt::Key_F10));main.addAction(&nativeClear);
        int nativeClears=0;QObject::connect(&nativeClear,&QAction::triggered,&main,[&]{++nativeClears;});TimelineViewportFixture::settle();
        QShortcutEvent event(QKeySequence(Qt::Key_F10),0,false);QApplication::sendEvent(&nativeClear,&event);TimelineViewportFixture::settle();
        check(nativeClears==1&&pool.view.itemDelegateForRow(0)&&!pool.view.itemDelegateForRow(1),"native Media Pool color shortcut releases only selected row paint and executes once");
    }
    colors.reset();presetCommandChecks(check,&main,commands,timeline,clip,files.path());
    std::printf("clip color menu failures=%d\n",failures);return failures?1:0;
}
