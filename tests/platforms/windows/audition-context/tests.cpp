// SPDX-License-Identifier: MIT
#include "trace.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>

enum FrameRequestEventSource { Clicked };
enum TransportControlMode { Play };
class UiFocusedWidgetManager : public QObject {
    Q_OBJECT
public: using QObject::QObject;
signals: void FocusChanged(int previous,int current);
};
class UiPlayerWidgetImp : public QWidget {
    Q_OBJECT
public: using QWidget::QWidget;
signals:
    void FocusReceived(FrameRequestEventSource source,bool refresh=true);
    void TransportControlTriggered(TransportControlMode mode,bool held);
    void IsActiveChanged(bool active);
    void MuteAudioChanged(bool muted);
};
class ClickTarget : public QWidget {
public: using QWidget::QWidget;
    int presses=0;
    bool event(QEvent *event) override {
        if (event->type()==QEvent::MouseButtonPress) {++presses;return true;}
        return QWidget::event(event);
    }
};
class TestApplication : public QApplication {
public: using QApplication::QApplication;
    int notifications=0;
    QPointer<QWidget> slot;
    QPointer<UiFocusedWidgetManager> manager;
    QPointer<UiPlayerWidgetImp> player;
    bool notify(QObject *object,QEvent *event) override {
        // Model the verified native notify -> context processing ordering, not
        // Resolve audio. This proves the proxy goes through QApplication notify.
        if (object && object->objectName()=="unboundAuditionContextProxy" && event->type()==QEvent::FocusIn) {
            ++notifications;
            auto widget=qobject_cast<QWidget *>(object);
            if (slot && widget && slot->isAncestorOf(widget) && manager && player) {
                emit manager->FocusChanged(100,200);
                emit player->IsActiveChanged(true);
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
int main(int argc,char **argv)
{
    TestApplication app(argc,argv);
    QTemporaryDir output;
    check(output.isValid(),"Temporary directory");
    qputenv("UNBOUND_AUDITION_TRACE_DIR",output.path().toUtf8());
    QWidget main; main.setObjectName("UiMainWindow");
    QWidget page(&main); page.setObjectName("WStackPage_Audio");
    QWidget slot(&page); slot.setObjectName("frameAudioPanelMediaPoolContainer");
    UiPlayerWidgetImp player(&slot); player.setObjectName("previewPlayer");
    UiPlayerWidgetImp timeline(&page); timeline.setObjectName("widgetAudioPagePlayer");
    UiFocusedWidgetManager manager; page.installEventFilter(&manager);
    app.slot=&slot; app.player=&player; app.manager=&manager;
    ClickTarget button(&player); button.setObjectName("btnTransportPlay");
    main.show();page.show();slot.show();player.show();timeline.show();button.show();app.processEvents();
    AuditionTrace trace;
    check(trace.begin(),"Start observer/intervention");
    click(&button);
    check(app.notifications==0 && button.presses==1,"Docked events unmodified");
    QWidget host(&slot,Qt::Window); host.setObjectName("unboundFloating_frameAudioPanelMediaPoolContainer");
    player.setParent(&host);host.show();player.show();button.show();app.processEvents();
    const auto before=app.notifications;
    QPointer<QWidget> focus=QApplication::focusWidget();
    QPointer<QWidget> active=QApplication::activeWindow();
    click(&button);
    check(app.notifications==before+1 && button.presses==2,"Exactly one relay and original click delivered");
    check(QApplication::focusWidget()==focus && QApplication::activeWindow()==active,"Physical Qt focus and active window preserved");
    check(player.window()==&host && !page.isAncestorOf(&player),"Content remains floating");
    auto proxy=slot.findChild<QWidget *>("unboundAuditionContextProxy",Qt::FindDirectChildrenOnly);
    check(proxy && proxy->isHidden() && proxy->window()==&main,"Invisible proxy on original branch");
    ClickTarget dock(&host);dock.setObjectName("resolvePaneUndockButton");dock.show();
    const auto after=app.notifications;click(&dock);
    check(app.notifications==after && dock.presses==1,"Dock control bypasses intervention");
    QWidget other(&page,Qt::Window);ClickTarget otherButton(&other);other.show();otherButton.show();
    click(&otherButton);check(app.notifications==after,"Other pane bypasses intervention");
    page.hide();click(&button);check(app.notifications==after,"Inactive page bypasses intervention");page.show();
    trace.finish("test-stop");
    click(&button);check(app.notifications==after,"Stop disables intervention");
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    check(!slot.findChild<QWidget *>("unboundAuditionContextProxy"),"Stop cleans up proxy");
    int ready=0,relays=0,contexts=0,activeSignals=0,stops=0;
    for (const auto &name:QDir(output.path()).entryList({"*.jsonl"},QDir::Files)) {
        QFile file(QDir(output.path()).filePath(name));check(file.open(QIODevice::ReadOnly),"Open capture");
        while (!file.atEnd()) {
            auto row=QJsonDocument::fromJson(file.readLine()).object();auto kind=row["kind"].toString();
            if (kind=="bridge-ready") {check(row["enabled"].toBool(),"Bridge ready");++ready;}
            if (kind=="bridge-after") {check(row["qtFocusUnchanged"].toBool() && row["activeWindowUnchanged"].toBool(),"Capture confirms focus preservation");++relays;}
            if (kind=="resolve-context") ++contexts;
            if (kind=="player-bool") ++activeSignals;
            if (kind=="stop") ++stops;
        }
    }
    check(ready==1 && relays>0 && contexts==relays && activeSignals==relays && stops==1,"Intervention and resulting signals logged");
    check(trace.begin(),"Restart intervention");
    const auto restarted=app.notifications;click(&button);check(app.notifications==restarted+1,"Restart resumes forwarding");
    for(int i=0;i<3000;++i) emit manager.FocusChanged(i,i+1);
    check(!trace.active(),"Log cap also stops intervention");
    const auto capped=app.notifications;click(&button);check(app.notifications==capped,"No forwarding after cap");
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    player.setParent(&slot);
    std::puts("PASS: native notify route, original input delivery, focus/window preservation, narrow scope, logs, stop/restart, cleanup and automatic cutoff");
}
#include "tests.moc"
