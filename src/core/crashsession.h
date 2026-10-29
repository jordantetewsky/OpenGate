// SPDX-License-Identifier: MIT
#pragma once
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include "diagnostics.h"
namespace Unbound {
// Postmortem correlation, not interception of Resolve's fatal signal handler.
class CrashSession : public QObject {
public:
    static QJsonObject assess(const QJsonObject &previous,const QString &archive,const QDateTime &now) {
        QJsonObject evidence{{"previous",previous},{"attribution","unknown"},
            {"native_report_time_match",false},{"meaning","An unclean exit alone does not establish a crash or its cause."}};
        const auto started=QDateTime::fromString(previous["started"].toString(),Qt::ISODateWithMs);
        if (!started.isValid()) return evidence;
        QDateTime newest;QString selected;
        for (const auto &part:archive.split("----------------------------------------------------------------------")) {
            auto match=QRegularExpression("#TIME (.*?) - Uptime").match(part);if (!match.hasMatch()) continue;
            auto time=QLocale::c().toDateTime(match.captured(1),"ddd MMM d HH:mm:ss yyyy");time.setTimeSpec(Qt::LocalTime);
            if (time.isValid() && time>=started && time<=now.addSecs(5) && (!newest.isValid() || time>newest)) { newest=time;selected=part; }
        }
        if (selected.isEmpty()) return evidence;
        QJsonArray frames;bool helper=false;
        for (const auto &line:selected.split('\n')) if (QRegularExpression("^\\d+\\s+").match(line).hasMatch()) {
            helper |= line.contains("UnboundWidgetProbe") || line.contains("Unbound.dvcp");if (frames.size()<50) frames.append(line);
        }
        evidence["native_report_time_match"]=true;evidence["native_crash_time"]=newest.toUTC().toString(Qt::ISODateWithMs);
        evidence["helper_frame_present"]=helper;evidence["frames"]=frames;
        evidence["meaning"]="Resolve's native crash report falls inside the previous session. Frame presence or absence does not by itself establish root cause.";
        return evidence;
    }
    explicit CrashSession(QObject *owner):QObject(owner) {
        directory=diagnosticDirectory();QDir().mkpath(directory);
        path=directory+"/active-session.json";QFile previousFile(path);
        if (previousFile.open(QIODevice::ReadOnly)) {
            auto previous=QJsonDocument::fromJson(previousFile.readAll()).object();previousFile.close();
            if (!previous.isEmpty() && previous["pid"].toDouble()!=QCoreApplication::applicationPid()) {
                QFile native(diagnosticResolveCrashArchive());QString archive;
                if (native.open(QIODevice::ReadOnly)) { native.seek(qMax<qint64>(0,native.size()-512*1024));archive=QString::fromUtf8(native.readAll()); }
                auto evidence=assess(previous,archive,QDateTime::currentDateTimeUtc());
                if (!previous["clean_shutdown"].toBool()) logEvent("helper.previous-session.unclean",{{"previous",previous},{"meaning","Unclean exit; causation unknown."}});
                if (!previous["clean_shutdown"].toBool() || evidence["native_report_time_match"].toBool()) {
                QDir().mkpath(directory+"/Incidents");
                const auto incident=directory+"/Incidents/"+QString::number(qint64(previous["pid"].toDouble()))+"-"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmsszzz")+".json";
                QSaveFile output(incident);if (output.open(QIODevice::WriteOnly)) { output.write(QJsonDocument(evidence).toJson());output.commit(); }
                logEvent("crash.previous-session.evidence",{{"incident",incident},{"evidence",evidence}});
                }
            }
        }
        QSaveFile marker(path);if (marker.open(QIODevice::WriteOnly)) {
            marker.write(QJsonDocument(QJsonObject{{"component","OpenGate"},{"session",diagnosticSessionId()},{"build",UNBOUND_BUILD_ID},{"pid",QCoreApplication::applicationPid()},
                {"started",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}).toJson());marker.commit();
        }
    }
    void markClean() {
        QFile input(path);if (!input.open(QIODevice::ReadOnly)) return;
        auto state=QJsonDocument::fromJson(input.readAll()).object();input.close();
        state["clean_shutdown"]=true;state["shutdown_at"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QSaveFile output(path);if (output.open(QIODevice::WriteOnly)) { output.write(QJsonDocument(state).toJson());output.commit(); }
        logEvent("helper.stop");
    }
private:
    QString directory,path;
};
}
