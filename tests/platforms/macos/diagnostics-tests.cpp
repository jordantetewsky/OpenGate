// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QEventLoop>
#include <QSet>
#include <QTemporaryDir>
#include <cstdio>
#include "diagnostics.h"
class FailingSink : public Unbound::DiagnosticSink
{
public:
    FailingSink() : DiagnosticSink(nullptr) {}
    qint64 failure = -1;
    int writeLimit = 0;
protected:
    qint64 writeBuffer(const QByteArray &data) override
    { return failure <= 0 ? failure : DiagnosticSink::writeBuffer(writeLimit ? data.left(writeLimit) : data); }
};
static QStringList storageWarnings;
static void captureStorageWarning(QtMsgType,const QMessageLogContext &,const QString &message)
{ storageWarnings.append(message); }
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv); QTemporaryDir files;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures = 0;
    const auto check = [&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    const auto read = [](const QString &path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); };
    const auto path = files.path()+"/events.jsonl";
    Unbound::logEvent("ui.responsiveness",{{"sequence",-1}});
    check(read(path).isEmpty(),"routine telemetry waits for the bounded flush");
    Unbound::logEvent("input.button.clicked",{{"sequence",-2}});
    check(read(path).count('\n') == 2,"button activation immediately flushes itself and preceding telemetry");
    const auto activation=QJsonDocument::fromJson(read(path).split('\n')[1]).object();
    check(!activation["session"].toString().isEmpty() && activation["event_sequence"].toInt()>0,
        "activation records carry session identity and ordered event identity");
    Unbound::logEvent("panel.undock.request",{{"sequence",-3}});
    check(read(path).count('\n') == 3,"ownership-change request is durable before the operation begins");
    const QString payload(262144,'x');
    for (int i=0;i<36;++i) Unbound::logEvent("rotation.fixture",{{"sequence",i},{"payload",payload}});
    Unbound::flushDiagnostics(); QSet<int> sequences;
    for (int part=0;part<=5;++part) {
        const auto data = read(path+(part ? QString(".%1").arg(part) : QString()));
        for (const auto &line : data.split('\n')) if (!line.isEmpty()) {
            const auto entry = QJsonDocument::fromJson(line).object();
            check(entry["component"] == "OpenGate" && entry["module"] == "UnboundWidgetProbe.dylib","each rotated record retains plugin identity");
            if (entry["event"] == "rotation.fixture") sequences.insert(entry["sequence"].toInt());
        }
    }
    check(sequences.size() == 36 && QFile::exists(path+".1"),"rotation preserves every complete record across the size boundary");
    Unbound::logEvent("telemetry.fixture",{{"sequence",1000}});
    const auto second = files.path()+"/second"; qputenv("UNBOUND_DIAGNOSTICS_DIR",second.toUtf8());
    Unbound::logEvent("input.action.triggered",{{"sequence",1001}});
    check(read(path).contains("1000") && read(second+"/events.jsonl").contains("1001"),"changing test paths flushes the old sink and routes the next action correctly");
    Unbound::logEvent("telemetry.fixture",{{"sequence",1002}});
    QEventLoop loop; QTimer::singleShot(100,&loop,&QEventLoop::quit); loop.exec();
    check(read(second+"/events.jsonl").contains("1002"),"idle timer flushes routine telemetry without another click");
    {
        const auto handler = qInstallMessageHandler(captureStorageWarning);
        FailingSink sink; const auto failurePath = files.path()+"/write-failure";
        for (int i=0;i<64;++i) sink.append(failurePath,QByteArray(65536,'x')+'\n',true);
        check(storageWarnings.size() == 1 && storageWarnings.first().startsWith("OpenGate diagnostics:"),
            "persistent storage failure identifies the plugin without flooding Resolve's log");
        sink.failure = 1; sink.append(failurePath,"recovered\n",true);
        check(read(failurePath+"/events.jsonl") == "recovered\n",
            "storage recovery writes the next record without retaining a growing failed backlog");
        sink.failure = 0; sink.append(failurePath,"zero-write\n",true);
        sink.failure = 1; sink.append(failurePath,"recovered-again\n",true);
        check(storageWarnings.size() == 2 && read(failurePath+"/events.jsonl") == "recovered\nrecovered-again\n",
            "zero-byte writes cannot accumulate telemetry and a later failure is reported again");
        sink.writeLimit = 1;
        for (int i=0;i<64;++i) sink.append(failurePath,QByteArray(65536,'x')+'\n',true);
        check(storageWarnings.join('\n').contains("backlog exceeded"),
            "persistently short writes cannot grow the telemetry backlog without a bound");
        qInstallMessageHandler(handler);
    }
    if (!failures) std::puts("PASS: immediate action/crash precursors, bounded telemetry flush and lossless log rotation");
    return failures ? 1 : 0;
}
