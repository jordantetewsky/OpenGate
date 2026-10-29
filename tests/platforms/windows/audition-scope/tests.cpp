// SPDX-License-Identifier: MIT
#include "bridge.h"
#include <QMouseEvent>
#include <cstdio>
#include <cstdlib>

class Target : public QWidget {
public:
    using QWidget::QWidget;
    int clicks=0,focusEvents=0,activations=0;
    bool event(QEvent *event) override {
        if (event->type()==QEvent::MouseButtonPress) {++clicks;return true;}
        if (event->type()==QEvent::FocusIn) ++focusEvents;
        if (event->type()==QEvent::WindowActivate) ++activations;
        return QWidget::event(event);
    }
};
class TestApplication : public QApplication {
public:
    using QApplication::QApplication;
    QWidget *main=nullptr,*preview=nullptr;
    int scope=0x5000000,context=-1,mainNotifications=0,targetNotifications=0;
    bool acceptMain=true;
    std::function<void()> onMain;
    bool notify(QObject *object,QEvent *event) override {
        if (object && object->isWidgetType()) {
            auto widget=static_cast<QWidget *>(object);
            if (widget==main && event->type()==QEvent::WindowActivate) {
                ++mainNotifications;if (acceptMain) scope=0x5000000;
                if (onMain) onMain();
            }
            if (event->type()==QEvent::FocusIn && preview && preview->isAncestorOf(widget)) {
                ++targetNotifications;
                if (scope==0x5000000 && widget->isVisible()) context=0x5010000;
            }
        }
        return QApplication::notify(object,event);
    }
    QJsonObject state() {return {{"valid",true},{"scope",scope},{"context",context}};}
};
static void check(bool condition,const char *message) {
    if (!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static void click(QWidget *widget) {
    QMouseEvent event(QEvent::MouseButtonPress,QPointF(2,2),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(widget,&event);
}
int main(int argc,char **argv)
{
    TestApplication app(argc,argv);
    Target main;QWidget page(&main),slot(&page);
    Target preview(&slot),play(&preview);
    play.setObjectName("btnTransportPlay");
    app.main=&main;app.preview=&preview;
    QWidget host(&slot,Qt::Window);host.setObjectName("unboundFloating_frameAudioPanelMediaPoolContainer");
    main.show();page.show();slot.show();host.show();
    preview.setParent(&host);preview.show();play.show();app.processEvents();
    QApplication::setActiveWindow(&host);play.setFocus();app.processEvents();
    QVector<QPair<QString,QJsonObject>> records;
    auto reporter=[&](const QString &kind,QJsonObject data){records.append({kind,data});};
    AuditionScopeBridge bridge(&main,&page,&preview,[&]{return app.state();},reporter);
    app.scope=0x7000000;app.context=-1;
    const int nativeMain=app.mainNotifications,nativeTarget=app.targetNotifications;
    main.activations=0;play.focusEvents=0;
    QPointer<QWidget> active=QApplication::activeWindow(),focus=QApplication::focusWidget();
    click(&play);
    check(app.mainNotifications==nativeMain+1 && app.targetNotifications==nativeTarget+1,"Main activation then visible target focus delivered through notify");
    check(app.scope==0x5000000 && app.context==0x5010000,"Model scope/context updated");
    check(play.clicks==1 && play.focusEvents==0 && main.activations==0,"Original click delivered, synthetic widget events consumed");
    check(QApplication::focusWidget()==focus && QApplication::activeWindow()==active,"Qt focus/active window preserved in fixture");
    check(preview.window()==&host,"Content remains floating");
    app.processEvents();
    bool settled=false;
    for (auto row:records) if (row.first=="scope-action-settled") settled=true;
    check(settled,"Automatic deferred capture");
    const int unchanged=app.mainNotifications;
    click(&play);check(app.mainNotifications==unchanged,"Correct scope does not retrigger activation");
    Target dock(&host);dock.setObjectName("resolvePaneUndockButton");dock.show();
    app.scope=0x7000000;click(&dock);check(app.mainNotifications==unchanged && dock.clicks==1,"Dock action bypassed");
    QWidget other(&page,Qt::Window);Target otherPlay(&other);otherPlay.setObjectName("btnTransportPlay");
    other.show();otherPlay.show();click(&otherPlay);check(app.mainNotifications==unchanged,"Other floating window bypassed");
    page.hide();click(&play);check(app.mainNotifications==unchanged,"Inactive page bypassed");page.show();
    // Other scopes must not be forcibly changed.
    QApplication::setActiveWindow(&host);app.scope=0x6000000;
    const auto beforeOtherScope=app.mainNotifications;click(&play);
    check(app.mainNotifications==beforeOtherScope && app.scope==0x6000000,"Unknown scope untouched");
    // If native activation fails to select Fairlight, do not send target focus.
    app.scope=0x7000000;app.acceptMain=false;
    const auto targetBeforeReject=app.targetNotifications;click(&play);
    check(app.targetNotifications==targetBeforeReject,"Unsuccessful scope switch does not replay focus");
    app.acceptMain=true;
    // Stop during nested native processing must still consume our exact event.
    app.scope=0x7000000;main.activations=0;
    app.onMain=[&]{bridge.stop();};click(&play);app.onMain={};
    check(!bridge.enabled() && main.activations==0,"Stop during notify retains synthetic-event consumption");
    const auto stopped=app.mainNotifications;app.scope=0x7000000;click(&play);
    check(app.mainNotifications==stopped,"No intervention after stop");
    // Docked playback still gets an automatic labelled capture, no intervention.
    preview.setParent(&slot);preview.show();play.show();QApplication::setActiveWindow(&main);app.processEvents();
    records.clear();
    AuditionScopeBridge restarted(&main,&page,&preview,[&]{return app.state();},reporter);
    const auto dockedCount=app.mainNotifications;click(&play);app.processEvents();
    check(app.mainNotifications==dockedCount,"Docked playback stays native");
    bool dockedLabel=false;
    for (auto row:records) if (row.first=="scope-action-settled" && row.second.value("phase")=="docked-audition") dockedLabel=true;
    check(dockedLabel,"Docked phase labelled automatically");
    restarted.stop();
    std::puts("PASS: scope notification order, native input preserved, exact synthetic event consumption, automatic captures, exclusions and stop during notify");
}
