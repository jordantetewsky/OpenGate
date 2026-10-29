// SPDX-License-Identifier: MIT
#pragma once
#include "settingsprofile.h"
#include "pageworkspacestate.h"
#include "starterworkspacegeometry.h"
#include "workspacedisplays.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QRect>

namespace Unbound {
class WorkspaceSettings
{
public:
    WorkspaceSettings() : path(qEnvironmentVariable("UNBOUND_WORKSPACE_FILE",
        settingsProfilePath("Workspace.json")))
    {
        QFile file(path);
        existed=file.exists();
        if(file.exists()) {
            if(!file.open(QIODevice::ReadOnly)){failure="Cannot read the saved workspace.";return;}
            QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
            if(error.error!=QJsonParseError::NoError || !document.isObject() ||
                (document.object().contains("version") && document.object()["version"]!=1)) {
                failure="Saved workspace is invalid or from an unsupported version.";return;
            }
            data=document.object();
        }
        data=fitStarterWorkspace(data);
        // Recover the last complete page snapshot, including if an earlier
        // process ended during an intermediate native restoration save.
        if(data["page_workspaces"].toObject().contains("edit")) {
            const auto pages=data["page_workspaces"].toObject();
            const auto edit=PageWorkspaceState::view(data,"edit");
            if(!edit.isEmpty()){data=edit;data["page_workspaces"]=pages;}
        }
    }
    bool hasSavedWorkspace() const {return existed;}
    QString error() const {return failure;}
    QJsonObject pageView(const QString &page) const {return PageWorkspaceState::view(data,page);}
    QString recordPage(const QString &page,const QJsonObject &value) {return PageWorkspaceState::record(data,page,value);}
    bool save() const
    {
        // Keep invalid/unknown files available for recovery instead of replacing
        // them with a default layout during ordinary startup callbacks.
        if(!failure.isEmpty())return false;
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;
        const QByteArray bytes = QJsonDocument(data).toJson();
        return file.write(bytes) == bytes.size() && file.commit();
    }
    static QJsonObject rectangle(const QRect &r)
    {
        return {{"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()}};
    }
    static QRect rectangle(const QJsonObject &object)
    {
        return object.contains("display") ? DisplayPlacement::resolve(object,workspaceDisplays()) : DisplayPlacement::rect(object);
    }
    static QJsonObject displayRectangle(const QRect &r,const QJsonObject &previous={})
    {return DisplayPlacement::capture(r,workspaceDisplays(),previous);}
    QJsonObject data;
private:
    QString path,failure;
    bool existed=false;
};
}
