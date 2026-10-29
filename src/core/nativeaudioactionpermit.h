// SPDX-License-Identifier: MIT
#pragma once
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>
namespace Unbound {
// One diagnostic authorization record, scoped by semantic Resolve identities.
// The consumed marker is atomically claimed before any native operation.
class NativeAudioActionPermit {
public:
    explicit NativeAudioActionPermit(QString path):path(std::move(path)){}
    bool valid()const{return !read().isEmpty();}
    bool used()const{return QFile::exists(path+".used");}
    bool matches(const QString &project,const QString &timeline,const QString &item)const {
        const auto permit=read();
        return !permit.isEmpty() && permit["project_id"].toString()==project &&
            permit["timeline_id"].toString()==timeline && permit["item_id"].toString()==item;
    }
    bool claim(const QString &project,const QString &timeline,const QString &item)const {
        if(!matches(project,timeline,item))return false;
        QFile marker(path+".used");
        if(!marker.open(QIODevice::WriteOnly|QIODevice::NewOnly))return false;
        marker.write("Native action invocation claimed once.\n");marker.close();return true;
    }
private:
    QString path;
    QJsonObject read()const {
        if(path.isEmpty() || QFileInfo(path).isSymLink())return {};
        QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};
        const auto bytes=file.read(8193);const auto permit=QJsonDocument::fromJson(bytes).object();
        const auto remaining=permit["expires_ms"].toDouble()-QDateTime::currentMSecsSinceEpoch();
        if(bytes.size()>8192 || permit["schema"].toInt()!=1 || permit["max_invocations"].toInt()!=1 ||
           !qIsFinite(remaining) || remaining<=0 || remaining>3600000 ||
           permit["project_id"].toString().isEmpty() || permit["timeline_id"].toString().isEmpty() || permit["item_id"].toString().isEmpty())return {};
        return permit;
    }
};
}
