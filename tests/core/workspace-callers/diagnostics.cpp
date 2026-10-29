// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <cstdio>
#include "../../../src/core/interactiondiagnostics.h"
#include "../../../src/core/crashsession.h"
#include "../../../src/core/surfacetrace.h"

int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
    QTemporaryDir directory; qputenv("UNBOUND_DIAGNOSTICS_DIR",directory.path().toUtf8());
    int failures=0;
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);
    auto layout=new QVBoxLayout(&main);
    auto button=new QPushButton("Inspector",&main);button->setObjectName("openGateInspector");layout->addWidget(button);
    auto editor=new QLineEdit(&main);editor->setObjectName("fixtureEditor");layout->addWidget(editor);
    auto action=new QAction("Show Inspector",&main);action->setObjectName("openGateShowInspector");main.addAction(action);
    Unbound::CrashSession journal(nullptr);
    Unbound::InteractionDiagnostics diagnostics(nullptr);
    Unbound::SurfaceTrace pointer(nullptr);
    QObject::connect(button,&QPushButton::clicked,&main,[&]{
        Unbound::logEvent("panel.visibility.request",{{"panel","inspector"},{"requested",true}});
        action->trigger();
        Unbound::logEvent("panel.visibility.complete",{{"panel","inspector"},{"visible",true}});
    });
    main.show();QApplication::processEvents();
    QMouseEvent press(QEvent::MouseButtonPress,QPointF(4,4),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,QPointF(4,4),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(button,&press);QApplication::sendEvent(button,&release);
    // Native/programmatic activation still emits the same observable signal.
    button->click();
    auto lazy=new QPushButton("Mixer",&main);lazy->setObjectName("openGateLazyMixer");layout->addWidget(lazy);
    QApplication::processEvents();lazy->click();
    QApplication::setActiveWindow(&main);editor->setFocus();QApplication::processEvents();
    check(QApplication::focusWidget()==editor,"fixture establishes the editor as input receiver");
    QKeyEvent key(QEvent::KeyPress,Qt::Key_X,Qt::NoModifier,"PRIVATE_TYPED_TEXT");
    QApplication::sendEvent(editor,&key);
    QWidget unrelated;unrelated.setObjectName("unrelatedWindow");
    QApplication::sendEvent(&unrelated,&press);
    QApplication::processEvents();pointer.flush();journal.markClean();Unbound::flushDiagnostics();
    QFile file(directory.path()+"/events.jsonl");check(file.open(QIODevice::ReadOnly),"event file exists");
    const auto bytes=file.readAll();
    QString session;int clicks=0,buttonSignals=0,actions=0,lazySignals=0,requests=0,complete=0,typed=0;double sequence=0;
    bool pointerPress=false,pointerRelease=false,nextTurn=false;
    for(const auto &line:bytes.split('\n')) {
        if(line.isEmpty())continue;
        const auto record=QJsonDocument::fromJson(line).object();const auto event=record["event"].toString();
        check(record["component"]=="OpenGate"&&record["vendor"]=="CineWaltz","every record identifies OpenGate");
        check(!record["module"].toString().isEmpty()&&!record["build"].toString().isEmpty(),"module and build accompany events");
        if(session.isEmpty())session=record["session"].toString();
        check(!session.isEmpty()&&record["session"]==session,"session correlates signals and transitions");
        check(record["event_sequence"].toDouble()>sequence,"event order is monotonic");sequence=record["event_sequence"].toDouble();
        clicks+=event=="input.click";actions+=event=="input.action.triggered";
        if(event=="input.button.clicked") {
            buttonSignals+=record["button"].toObject()["name"]=="openGateInspector";
            lazySignals+=record["button"].toObject()["name"]=="openGateLazyMixer";
        }
        requests+=event=="panel.visibility.request";complete+=event=="panel.visibility.complete";
        if(event=="input.pointer.batch")for(const auto &sample:record["samples"].toArray()) {
            pointerPress|=sample.toObject()["event"].toInt()==QEvent::MouseButtonPress;
            pointerRelease|=sample.toObject()["event"].toInt()==QEvent::MouseButtonRelease;
        }
        nextTurn|=event=="input.surface.next-turn";
        if(event=="input.key.observed"&&record["text_editing"].toBool()) {
            ++typed;check(!record.contains("key")&&!record.contains("text")&&!record.contains("modifiers"),"typing values are excluded");
        }
        check(record["receiver"].toObject()["name"]!="unrelatedWindow","unrelated windows excluded");
    }
    check(clicks>=1&&buttonSignals==2&&actions==2&&requests==2&&complete==2,"pointer and programmatic signals retain transition trail without duplicate connections");
    check(lazySignals==1,"lazily created buttons observed");check(typed>=1&&!bytes.contains("PRIVATE_TYPED_TEXT"),"text editing recorded without contents");
    check(pointerPress&&pointerRelease&&nextTurn,"same bounded pointer and next-turn trail is present on both platforms");
    QFile marker(directory.path()+"/active-session.json");marker.open(QIODevice::ReadOnly);
    check(QJsonDocument::fromJson(marker.readAll()).object()["clean_shutdown"].toBool(),"session journal records clean shutdown");
    std::printf("shared diagnostic failures=%d\n",failures);return failures?1:0;
}
