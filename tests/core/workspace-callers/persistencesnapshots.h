// SPDX-License-Identifier: MIT
#pragma once
#include <QFile>
#include <QJsonDocument>

// Optional before/after evidence from the assembled production flow. Keep full
// settings and named records; the comparison normalizes generated preset IDs only.
inline void recordPersistenceSnapshot(const QString &stage) {
    const auto path=qEnvironmentVariable("OPENGATE_PERSISTENCE_CAPTURE");
    if(path.isEmpty())return;
    QJsonObject snapshots;
    QFile file(path);
    if(file.open(QIODevice::ReadOnly)){snapshots=QJsonDocument::fromJson(file.readAll()).object();file.close();}
    Unbound::WorkspaceSettings settings;Unbound::ArrangementStore arrangements;
    snapshots[stage]=QJsonObject{{"settings",settings.data},{"arrangements",arrangements.list()}};
    if(!file.open(QIODevice::WriteOnly)||file.write(QJsonDocument(snapshots).toJson())<0)
        qFatal("Cannot write persistence comparison evidence");
}
