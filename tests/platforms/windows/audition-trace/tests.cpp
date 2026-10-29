// SPDX-License-Identifier: MIT
#include "trace.h"
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QVBoxLayout>
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
static void check(bool condition,const char *message)
{
    if (!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    QTemporaryDir output;
    check(output.isValid(),"Temporary directory");
    qputenv("UNBOUND_AUDITION_TRACE_DIR",output.path().toUtf8());
    QWidget main; main.setObjectName("UiMainWindow"); main.resize(900,700);
    QWidget page(&main); page.setObjectName("WStackPage_Audio"); page.resize(800,600);
    QWidget slot(&page); slot.setObjectName("frameAudioPanelMediaPoolContainer");
    UiPlayerWidgetImp player(&slot); player.setObjectName("previewPlayer");
    UiPlayerWidgetImp timeline(&page); timeline.setObjectName("widgetAudioPagePlayer");
    UiFocusedWidgetManager manager;
    page.installEventFilter(&manager); // Parentless native manager discovery.
    main.show(); page.show(); slot.show(); player.show(); timeline.show(); app.processEvents();
    AuditionTrace trace;
    check(trace.begin(),"Start trace");
    emit manager.FocusChanged(17,23);
    emit player.IsActiveChanged(true);
    emit player.MuteAudioChanged(false);
    emit player.FocusReceived(Clicked,true);
    emit player.TransportControlTriggered(Play,false);
    trace.checkpoint();
    QWidget child(&page);
    player.setParent(&child); child.show(); player.show(); app.processEvents(); trace.checkpoint();
    QWidget floating(&slot,Qt::Window);
    player.setParent(&floating); floating.show(); player.show(); app.processEvents(); trace.checkpoint();
    trace.finish("test-stop");
    check(!trace.active(),"Stop detaches observer");
    auto files=QDir(output.path()).entryList({"*.jsonl"},QDir::Files);
    check(files.size()==1,"One log file");
    QFile file(QDir(output.path()).filePath(files.first())); check(file.open(QIODevice::ReadOnly),"Read log");
    int contexts=0,bools=0,events=0,connected=0,childStates=0,floatingStates=0;
    while (!file.atEnd()) {
        const auto record=QJsonDocument::fromJson(file.readLine()).object();
        const auto kind=record.value("kind").toString();
        if (kind=="resolve-context") {++contexts;check(record["old"].toInt()==17 && record["new"].toInt()==23,"Context payload");}
        if (kind=="player-bool") ++bools;
        if (kind=="player-event") ++events;
        if (kind=="connection") {check(record["connected"].toBool(),"All verified signals connected");++connected;}
        if (kind=="checkpoint") for (auto p:record["state"].toObject()["players"].toArray()) {
            auto w=p.toObject(); if (w["name"]!="previewPlayer") continue;
            if (w["mainIsWindow"].toBool() && w["pageIsAncestor"].toBool()) ++childStates;
            if (!w["mainIsWindow"].toBool() && !w["pageIsAncestor"].toBool()) ++floatingStates;
        }
    }
    check(contexts==1 && bools==2 && events==2 && connected==9,"Signal records, no duplicate FocusReceived clones");
    check(childStates>=2 && floatingStates==1,"Window boundary and ancestry captured");
    const auto originalSize=file.size();
    emit manager.FocusChanged(23,24); trace.checkpoint();
    check(file.size()==originalSize,"Stopped capture stays stopped"); file.close();
    check(trace.begin(),"Restart capture");
    QEvent paint(QEvent::Paint);
    for (int i=0;i<4000;++i) QApplication::sendEvent(&page,&paint);
    check(trace.active(),"Paint events ignored");
    for (int i=0;i<3000;++i) emit manager.FocusChanged(i,i+1);
    check(!trace.active(),"Record cap terminates capture");
    player.setParent(&slot);
    std::puts("PASS: context IDs, player signals, same-window/independent-window snapshots, stop/restart, no paint logging and hard record cap");
}
#include "tests.moc"
