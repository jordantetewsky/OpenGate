// SPDX-License-Identifier: MIT
#pragma once
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
// Existing Mac single-use launch policy, shared with the Windows service.
// Start watching before controller construction; consume startup once ready.
class ControlLaunchRequests : public QObject {
public:
    ControlLaunchRequests(QObject *owner,QString directory,std::function<void()> show)
        :QObject(owner),directory(std::move(directory)),show(std::move(show)),watcher(this) {
        QDir().mkpath(this->directory);watcher.addPath(this->directory);
        connect(&watcher,&QFileSystemWatcher::directoryChanged,this,[this]{consume(false);});
    }
    void consume(bool startup) {
        QFile request(directory+QStringLiteral("/show-controls.request"));
        if (!request.open(QIODevice::ReadOnly)) return;
        const auto age=QFileInfo(request).lastModified().msecsTo(QDateTime::currentDateTimeUtc());
        const auto token=request.readAll(); request.close();
        // Consume stale files too; a recent explicit launch may precede startup.
        request.remove();
        if (token.isEmpty() || token==lastRequest) return;
        lastRequest=token;
        if (startup && (age<0 || age>15000)) {
            logEvent("controls.launch.stale",{{"age_ms",double(age)}}); return;
        }
        logEvent("controls.launch.request",{{"request",QString::fromUtf8(token)},{"startup",startup}});
        if(show)show();
    }
private:
    QString directory;std::function<void()> show;QFileSystemWatcher watcher;QByteArray lastRequest;
};
}
