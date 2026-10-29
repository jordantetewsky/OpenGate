// SPDX-License-Identifier: MIT
#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QTimer>
#include <QThread>
#include <QDebug>
namespace Unbound {
// One GUI-thread sink keeps file setup off the input path. Clicks, ownership
// changes and lifecycle records flush immediately; bulk telemetry flushes at 50 ms.
class DiagnosticSink : public QObject
{
public:
    explicit DiagnosticSink(QObject *owner) : QObject(owner)
    {
        timer.setSingleShot(true); timer.setInterval(50);
        connect(&timer,&QTimer::timeout,this,[this] { flush(); });
        connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,this,[this] { flush(); });
    }
    ~DiagnosticSink() override { flush(); }
    void append(const QString &directory,const QByteArray &line,bool immediate)
    {
        const auto next = directory+"/events.jsonl";
        if (next != path || !file.isOpen()) {
            flush(); file.close(); path = next; bytes = 0;
            if (!QDir().mkpath(directory)) { failed(); return; }
            file.setFileName(path);
            if (!file.open(QIODevice::WriteOnly|QIODevice::Append)) { failed(); return; }
            bytes = file.size();
        }
        constexpr int maximumPendingBytes = 1024*1024;
        if (buffer.size()+line.size() > maximumPendingBytes) { flush(); if (!file.isOpen()) return; }
        if (buffer.size()+line.size() > maximumPendingBytes) { failed("telemetry backlog exceeded 1 MiB"); return; }
        buffer.append(line);
        if (immediate || buffer.size() >= 65536) flush();
        else if (!timer.isActive()) timer.start();
    }
    void flush()
    {
        timer.stop(); if (buffer.isEmpty() || !file.isOpen()) return;
        if (bytes+buffer.size() > 8*1024*1024) {
            file.flush(); file.close(); QFile::remove(path+".5");
            for (int i=4;i>=1;--i) QFile::rename(path+QString(".%1").arg(i),path+QString(".%1").arg(i+1));
            QFile::rename(path,path+".1");
            if (!file.open(QIODevice::WriteOnly|QIODevice::Append)) { failed(); return; }
            bytes = file.size();
        }
        const auto written = writeBuffer(buffer);
        if (written > 0) { bytes += written; buffer.remove(0,int(written)); }
        if (written <= 0 || !file.flush()) { failed(); return; }
        warned = false;
        if (!buffer.isEmpty()) timer.start();
    }
protected:
    virtual qint64 writeBuffer(const QByteArray &data) { return file.write(data); }
private:
    void failed(const char *reason = "storage write failed")
    {
        // A full disk must not turn telemetry into an unbounded GUI allocation.
        // Keep a component-named warning in Qt/Resolve's own diagnostic stream.
        buffer.clear(); timer.stop(); file.close();
        if (!warned) qWarning("OpenGate diagnostics: %s; buffered telemetry dropped. Retrying on the next record.",reason);
        warned = true;
    }
    QFile file;
    QByteArray buffer;
    QString path;
    qint64 bytes = 0;
    QTimer timer;
    bool warned = false;
};
inline QPointer<DiagnosticSink> &diagnosticSink()
{
    static QPointer<DiagnosticSink> sink;
    return sink;
}
inline bool immediateDiagnostic(const QString &event)
{
    return event.startsWith("workspace.edit.") || event.startsWith("crash.") || event.startsWith("input.click") || event.startsWith("input.button") || event == "input.action.triggered" ||
        event.startsWith("button.") || event.startsWith("helper.") || event.startsWith("workspace.menu") ||
        event.contains("request") || event.contains("complete") || event.contains("failed") || event.contains("error") ||
        event == "drag.begin" || event == "drag.end" || event == "arrangement.result" || event == "group.drop.result";
}
inline void writeDiagnostic(const QString &directory,const QString &event,const QByteArray &line)
{
    if (!QCoreApplication::instance() || QCoreApplication::closingDown() ||
        QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QDir().mkpath(directory); QFile file(directory+"/events.jsonl");
        if (file.open(QIODevice::WriteOnly|QIODevice::Append)) file.write(line);
        return;
    }
    auto &sink = diagnosticSink(); if (!sink) sink = new DiagnosticSink(QCoreApplication::instance());
    sink->append(directory,line,immediateDiagnostic(event));
}
inline void flushDiagnostics() { if (diagnosticSink()) diagnosticSink()->flush(); }
}
