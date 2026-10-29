// SPDX-License-Identifier: MIT
#pragma once
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>
#include <QSaveFile>
#include <QTimer>
#include <QUuid>
#include "nativeworkflowoperation.h"
namespace Unbound {
// Correlates one read-only Python planning request. Owns no clip or UI state.
class AudioChannelPlanRequest : public QObject {
public:
    explicit AudioChannelPlanRequest(QObject *owner,bool retainCompletedTicket=false):QObject(owner),watcher(this),retainCompletedTicket(retainCompletedTicket) {
        base=qEnvironmentVariable("UNBOUND_LAUNCH_DIRECTORY",QDir::homePath()+"/Library/Logs/OpenGate");
#ifdef Q_OS_WIN
        base=qEnvironmentVariable("UNBOUND_LAUNCH_DIRECTORY",qEnvironmentVariable("LOCALAPPDATA")+"/OpenGate/Logs");
#endif
        QDir().mkpath(base+"/audio-channel-plans");watcher.addPath(base+"/audio-channel-plans");
        connect(&watcher,&QFileSystemWatcher::directoryChanged,this,[this]{consume();});
    }
    ~AudioChannelPlanRequest()override{cancel();}
    bool start(int direction,bool skipExcluded,std::function<void(QJsonObject)> completion) {
        if(!token.isEmpty())return false;
        token=QUuid::createUuid().toString(QUuid::WithoutBraces);done=std::move(completion);
        const auto command=QString(direction<0?"edit.audio.previous-channel":"edit.audio.next-channel")+
            (skipExcluded?"":"-all-source");
        QSaveFile file(base+"/audio-channel-plan-job.json");
        if(!file.open(QIODevice::WriteOnly)){cancel();return false;}
        file.write(QJsonDocument(QJsonObject{{"protocol","native-audio-plan-v3"},{"token",token},{"command",command},
            {"issued_ms",double(QDateTime::currentMSecsSinceEpoch())}}).toJson(QJsonDocument::Compact));
        if(!file.commit()){cancel();return false;}
        const auto label=QString(direction<0?"OpenGate - Previous Audio Channel":"OpenGate - Next Audio Channel")+
            (skipExcluded?" — Skip Excluded Channels":" — All Source Channels");
        const bool invoked=NativeWorkflowOperation::invoke("audio-channel-plan",label,
            "# opengate-command-route: native-audio-plan-v3",[](int,const QString &){return false;});
        if(!invoked){cancel();return false;}
        QTimer::singleShot(0,this,[this]{consume();});return true;
    }
    void cancel(bool retainCancelledTicket=true) {
        if(token.isEmpty())return;
        QFile job(base+"/audio-channel-plan-job.json");
        if(job.open(QIODevice::ReadOnly)) {
            const auto data=QJsonDocument::fromJson(job.read(4096)).object();job.close();
            if(data["token"].toString()==token) {
                if(retainCancelledTicket) {
                    auto cancelled=data;cancelled["inactive"]=true;
                    QSaveFile tombstone(job.fileName());
                    if(tombstone.open(QIODevice::WriteOnly)) {tombstone.write(QJsonDocument(cancelled).toJson(QJsonDocument::Compact));tombstone.commit();}
                } else job.remove();
            }
        }
        QFile::remove(base+"/audio-channel-plans/"+token+".json");token.clear();done={};
    }
private:
    QString base,token;QFileSystemWatcher watcher;bool retainCompletedTicket=false;std::function<void(QJsonObject)> done;
    void consume() {
        if(token.isEmpty())return;
        QFile file(base+"/audio-channel-plans/"+token+".json");
        if(!file.open(QIODevice::ReadOnly))return;
        const auto bytes=file.read(32769);file.close();file.remove();
        auto result=QJsonDocument::fromJson(bytes).object();
        const auto age=QDateTime::currentMSecsSinceEpoch()-result["issued_ms"].toDouble(-1);
        if(bytes.size()>32768 || result["protocol"].toString()!="native-audio-plan-v3" || result["token"].toString()!=token || age<0 || age>2000)
            result={{"success",false},{"reason","invalid-or-expired-audio-plan"}};
        auto callback=done;cancel(retainCompletedTicket);if(callback)callback(result);
    }
};
}
