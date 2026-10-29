// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <algorithm>

// Fixture-only attribution. Measure outer event deliveries so nested signals and
// sendEvent calls are not counted twice. No per-event logging or disk I/O.
class EventWorkApplication : public QApplication {
public:
    using QApplication::QApplication;
    void setScriptedPointer(bool held) { scriptedPointer=held; }
    int ignoredPointerMoves() const { return unsolicitedPointerMoves; }
    void beginProbe() { records.clear(); probing=true; }
    void setProbeEnabled(bool enabled) { probing=enabled; }
    QJsonArray endProbe() {
        probing=false;
        QList<QString> keys=records.keys();
        std::sort(keys.begin(),keys.end(),[this](const QString &a,const QString &b) { return records[a].total>records[b].total; });
        QJsonArray result;
        for (const auto &key : keys.mid(0,12)) {
            const auto record=records.value(key);
            result.append(QJsonObject{{"receiver_class_and_event",key},{"count",double(record.count)},
                {"total_ms",record.total/1000000.0},{"max_ms",record.maximum/1000000.0}});
        }
        return result;
    }
    bool notify(QObject *receiver,QEvent *event) override {
        // A programmatically moved native window can synthesize a hover move
        // with physical buttons=None. It is not part of our injected held-button
        // stream. Production still receives these events and cancels normally.
        if (scriptedPointer && event->type()==QEvent::MouseMove &&
            !static_cast<QMouseEvent *>(event)->buttons().testFlag(Qt::LeftButton)) {
            ++unsolicitedPointerMoves; return true;
        }
        const bool sample=probing && depth==0;
        // Object names here come only from this synthetic fixture, never from
        // an editor's project, text fields or media names.
        const QString key=sample ? QString::fromLatin1(receiver->metaObject()->className())+"["+
            receiver->objectName()+"]:"+QString::number(int(event->type())) : QString();
        QElapsedTimer clock; if (sample) clock.start();
        ++depth;
        const bool result=QApplication::notify(receiver,event);
        --depth;
        if (sample && records.size()<256) {
            const auto elapsed=clock.nsecsElapsed(); auto &record=records[key];
            ++record.count; record.total+=elapsed; record.maximum=qMax(record.maximum,elapsed);
        }
        return result;
    }
private:
    struct Record { quint64 count=0; qint64 total=0,maximum=0; };
    QHash<QString,Record> records;
    int depth=0;
    bool probing=false,scriptedPointer=false;
    int unsolicitedPointerMoves=0;
};
