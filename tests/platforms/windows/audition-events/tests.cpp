// SPDX-License-Identifier: MIT
#include "bridge.h"
#include <QFocusEvent>
#include <QMouseEvent>
#include <QStringList>
#include <cstdio>
#include <cstdlib>

class Target : public QWidget {
public:
    using QWidget::QWidget;
    int activations=0,clicks=0,focusEvents=0;
    bool event(QEvent *event) override {
        if (event->type()==QEvent::WindowActivate) ++activations;
        if (event->type()==QEvent::FocusIn) ++focusEvents;
        if (event->type()==QEvent::MouseButtonPress) {++clicks;return true;}
        return QWidget::event(event);
    }
};
class TestApplication : public QApplication {
public:
    using QApplication::QApplication;
    QWidget *main=nullptr,*preview=nullptr,*host=nullptr;
    int scope=0,context=-1,mainNotifications=0;
    QStringList sequence;
    std::function<void()> onMain;
    bool notify(QObject *object,QEvent *event) override {
        if (object && object->isWidgetType()) {
            auto widget=static_cast<QWidget *>(object);
            // Model the observed application-level scope selection. These fake
            // fields exist ONLY in this fixture, never in the implementation.
            if (event->type()==QEvent::WindowActivate && widget->isWindow()) {
                scope=widget==main ? 5 : 7;
                if (widget==main) {++mainNotifications;sequence.append("main");if(onMain) onMain();}
                else if (widget==host) sequence.append("host");
            }
            if (event->type()==QEvent::FocusIn && preview && preview->isAncestorOf(widget) && widget->isVisible()) {
                sequence.append("focus");context=scope==5 ? 501 : -1;
            }
        }
        return QApplication::notify(object,event);
    }
};
static void check(bool condition,const char *message) {
    if (!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static void click(QWidget *widget) {
    QMouseEvent event(QEvent::MouseButtonPress,QPointF(2,2),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(widget,&event);
}
static void activateEvent(QWidget *widget) {
    QEvent event(QEvent::WindowActivate);QApplication::sendEvent(widget,&event);
}
int main(int argc,char **argv)
{
    TestApplication app(argc,argv);
    Target main;QWidget page(&main),slot(&page);
    Target preview(&slot),play(&preview),mainChild(&main);
    Target host(&slot,Qt::Window);host.setObjectName("unboundFloating_frameAudioPanelMediaPoolContainer");
    app.main=&main;app.preview=&preview;app.host=&host;
    preview.setFocusPolicy(Qt::StrongFocus);play.setFocusPolicy(Qt::StrongFocus);
    main.show();page.show();slot.show();mainChild.show();host.show();
    preview.setParent(&host);preview.show();play.show();app.processEvents();
    QApplication::setActiveWindow(&main);app.processEvents();
    QVector<QPair<QString,QJsonObject>> records;
    auto reporter=[&](const QString &kind,QJsonObject data){records.append({kind,data});};
    AuditionActivationBridge bridge(&main,&page,&preview,reporter);
    app.sequence.clear();main.activations=0;mainChild.activations=0;
    const int before=app.mainNotifications;
    QApplication::setActiveWindow(&host);app.processEvents();
    check(app.mainNotifications==before+1 && app.scope==5,"Host activation triggers exactly one native main notification");
    check(main.activations==0 && mainChild.activations==0,"Synthetic event consumed before main widget/child delivery");
    check(QApplication::activeWindow()==&host && preview.window()==&host,"Host remains active and content remains floating");
    const int after=app.mainNotifications;
    // Simulate genuine subsequent focus if the offscreen plugin did not already
    // restore it. The bridge must never synthesize one itself.
    play.setFocus(Qt::MouseFocusReason);app.processEvents();
    check(app.context==501,"Genuine visible target focus uses corrected scope");
    check(app.sequence.indexOf("host")>=0 && app.sequence.indexOf("main")>app.sequence.indexOf("host") &&
        app.sequence.lastIndexOf("focus")>app.sequence.indexOf("main"),"Activation correction precedes genuine focus");
    for(int i=0;i<20;++i) click(&play);
    QFocusEvent focus(QEvent::FocusIn,Qt::MouseFocusReason);QApplication::sendEvent(&play,&focus);
    check(app.mainNotifications==after && play.clicks==20,"Repeated clicks/focus cause no correction");
    activateEvent(&play);
    check(app.mainNotifications==after,"Propagated child activation ignored");
    // Each root activation must be handled, even for the same HWND: native scope
    // selection runs again on every such event. Do not deduplicate away a reset.
    activateEvent(&host);check(app.mainNotifications==after+1 && app.scope==5,"Repeated real root activation handled once");
    const int repeated=app.mainNotifications;
    QWidget other(&page,Qt::Window);other.show();QApplication::setActiveWindow(&other);app.processEvents();
    check(app.mainNotifications==repeated,"Other floating windows unchanged");
    QApplication::setActiveWindow(&main);const int onMain=app.mainNotifications;
    activateEvent(&host);check(app.mainNotifications==onMain,"Inactive host activation ignored");
    QApplication::setActiveWindow(&host);page.hide();const int hidden=app.mainNotifications;
    activateEvent(&host);check(app.mainNotifications==hidden,"Hidden page excluded");page.show();
    preview.hide();activateEvent(&host);check(app.mainNotifications==hidden,"Hidden player excluded");preview.show();
    // Scope implementation has no direct reads; an unknown host name is excluded.
    host.setObjectName("unrelatedWindow");activateEvent(&host);
    check(app.mainNotifications==hidden,"Unrecognized host excluded");
    host.setObjectName("unboundFloating_frameAudioPanelMediaPoolContainer");
    // Stop inside native application handling still consumes the synthetic event.
    main.activations=0;app.onMain=[&]{bridge.stop();};activateEvent(&host);app.onMain={};
    check(!bridge.enabled() && main.activations==0,"Stop during notification safely suppresses widget activation");
    const int stopped=app.mainNotifications;activateEvent(&host);
    check(app.mainNotifications==stopped && app.scope==7,"Stopped bridge leaves native behavior untouched");
    preview.setParent(&slot);preview.show();play.show();QApplication::setActiveWindow(&main);app.processEvents();
    AuditionActivationBridge docked(&main,&page,&preview,reporter);
    const int native=app.mainNotifications;activateEvent(&main);
    check(app.mainNotifications==native+1,"Docked/main activation is never duplicated");
    // Reparent into a NEW host to exercise retained controller references.
    Target secondHost(&slot,Qt::Window);secondHost.setObjectName("unboundFloating_frameAudioPanelMediaPoolContainer");
    preview.setParent(&secondHost);secondHost.show();preview.show();play.show();app.processEvents();
    QApplication::setActiveWindow(&main);const int redetach=app.mainNotifications;
    QApplication::setActiveWindow(&secondHost);app.processEvents();
    check(app.mainNotifications==redetach+1,"New host after redocking uses current public window identity");
    docked.stop();preview.setParent(&slot);
    for(auto record:records) if(record.first=="activation-after")
        check(record.second.value("activeWindowUnchanged").toBool(),"Correction does not change active window in fixture");
    std::puts("PASS: public-event-only correction, activation-before-focus order, no work on clicks, no propagated activation, exclusions, stop and new-host lifecycle");
}
