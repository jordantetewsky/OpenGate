#include <QApplication>
#include <QTemporaryDir>
#include <QMouseEvent>
#include <QThread>
#include <cstdio>
#include "surfacetrace.h"
#include "clipcolorprobe.h"
#include "keyboardprobe.h"
#include "boundarydiagnostics.h"
#include "nativepaneldrag.h"
static int failures=0;
static void check(bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } }
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir temporary;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",temporary.path().toUtf8());
    {
        Unbound::SurfaceTrace pointer(nullptr); Unbound::ClipColorProbe colors(nullptr);
        Unbound::KeyboardProbe keyboard(nullptr); Unbound::BoundaryDiagnostics boundaries(nullptr);
        QWidget main; main.setObjectName("UiMainWindow");
        QWidget surface(&main); surface.setObjectName("nativeInputSurface");
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(4,5),QPointF(104,205),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&surface,&press);
        for (int i=0;i<1000;++i) {
            QMouseEvent move(QEvent::MouseMove,QPointF(i,5),QPointF(104+i,205),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(&surface,&move);
        }
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(6,5),QPointF(106,205),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(&surface,&release);
        pointer.flush();
        QMenu palette; palette.setObjectName("markSetClipColor");
        auto orange = palette.addAction("Orange"); orange->setData(1);
        auto privateLabel = palette.addAction("PRIVATE_FIXTURE_TEXT"); privateLabel->setProperty("identity",QStringLiteral("PRIVATE_PROPERTY_VALUE"));
        QEvent show(QEvent::Show); QApplication::sendEvent(&palette,&show);
        QWidget editor; editor.setObjectName("shortcutCustomizationTest"); QApplication::sendEvent(&editor,&show);
        Unbound::NativePanelDrag drag(nullptr); drag.begin("fixture-panel");
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
        QApplication::sendEvent(&editor,&escape);
        check(!drag.waiting(),"Escape cancels pending host capture before a delayed undock completes");
        QApplication::processEvents();
    }
    Unbound::diagnosticSink()->flush();
    QFile file(temporary.path()+"/events.jsonl"); check(file.open(QIODevice::ReadOnly),"diagnostics file exists");
    const auto bytes=file.readAll(); int paletteRecords=0,keyboardRecords=0,pointerBatches=0; bool coalesced=false,nextTurn=false,position=false;
    for (const auto &line : bytes.split('\n')) {
        const auto item=QJsonDocument::fromJson(line).object();
        // logEvent stores payload fields at the top level.
        const auto event=item["event"].toString();
        if (event == "capability.clip-color.palette") ++paletteRecords;
        if (event == "capability.keyboard.interface") ++keyboardRecords;
        if (event == "input.pointer.batch") {
            ++pointerBatches; coalesced |= item["coalesced_moves"].toInt()>0;
            for (const auto &sample : item["samples"].toArray())
                position |= sample.toObject()["x"].toInt()==104 && sample.toObject()["y"].toInt()==205;
        }
        nextTurn |= event == "input.surface.next-turn";
    }
    check(paletteRecords==1 && keyboardRecords==1,"capability probes inventory only their recognized surfaces");
    check(pointerBatches>0 && coalesced && position && nextTurn,"pointer trace records positions, coalescing and next-turn timing");
    check(!bytes.contains("PRIVATE_FIXTURE_TEXT") && !bytes.contains("PRIVATE_PROPERTY_VALUE"),"capability probes redact unrecognized labels and property values");
    if (!failures) std::puts("PASS: bounded pointer tracing and read-only capability probe records");
    return failures ? 1 : 0;
}
