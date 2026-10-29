// SPDX-License-Identifier: MIT
#pragma once
#include <QFileSystemWatcher>
#include <QApplication>
#include <QSet>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTimer>
#include <functional>
#include "diagnostics.h"
#include "../platform/nativewindowcontract.h"
namespace Unbound {
// Workflow Integration scripts are native shortcut targets. Their single-use
// requests dispatch whitelisted command IDs back on Resolve's Qt GUI thread.
class WorkflowCommandBridge : public QObject {
public:
    WorkflowCommandBridge(QObject *owner,const QString &launchDirectory,std::function<bool(QString)> invoke)
        : QObject(owner),directory(launchDirectory+"/command-requests"),dispatch(std::move(invoke)),watcher(this) {
        QDir().mkpath(directory); watcher.addPath(directory);
        connect(&watcher,&QFileSystemWatcher::directoryChanged,this,[this] { queue(); });
        auto wake=watchNativeCommandRequests(this,[this] { queue(); });
        logEvent("command.workflow.bridge",{{"native_wake",wake->property("openGateNativeWakeReady").toBool()},
            {"fallback","filesystem-watcher"}});
        queue();
    }
private:
    QString directory; std::function<bool(QString)> dispatch; QFileSystemWatcher watcher; bool queued=false; QSet<QString> deferred;
    void queue(int delay=0) {
        if (queued) return; queued=true;
        QTimer::singleShot(delay,this,[this] { queued=false; consume(); });
    }
    void consume() {
        auto files=QDir(directory).entryInfoList({"*.json"},QDir::Files|QDir::NoSymLinks,QDir::Time|QDir::Reversed);
        int processed=0;
        for (const auto &info:files) {
            if (++processed>16) { queue(); break; }
            QFile file(info.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly)) continue;
            auto bytes=file.read(4097); file.close();
            const auto object=QJsonDocument::fromJson(bytes).object();
            const auto id=object["command"].toString();
            const auto issued=object["issued_ms"].toDouble(-1);
            const auto age=QDateTime::currentMSecsSinceEpoch()-issued;
            const bool valid=bytes.size()<=4096 && !id.isEmpty() && qIsFinite(age) && age>=0 && age<=15000;
            // Native workflow handlers can publish inside AppKit's menu-tracking
            // loop. Retain the request until the popup closes, rather than reject
            // a valid command or mutate/close the user's native menu ourselves.
            if(valid && QApplication::activePopupWidget()) {
                if(!deferred.contains(info.absoluteFilePath())) {
                    deferred.insert(info.absoluteFilePath());
                    logEvent("command.workflow.deferred",{{"command",id},{"reason","menu-tracking"}});
                }
                queue(25);return;
            }
            // Claim only when eligible for dispatch (or expired/invalid).
            if(!file.remove())continue;
            deferred.remove(info.absoluteFilePath());
            const bool accepted=valid && dispatch && dispatch(id);
            logEvent("command.workflow.request",{{"command",id},{"valid",valid},{"accepted",accepted},{"age_ms",age}});
        }
    }
};
}
