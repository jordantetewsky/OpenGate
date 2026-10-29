// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonParseError>
#include <QSet>
#include <QUuid>
#include "workspacesettings.h"
#include "workspacepages.h"
#include "workspacepresetpolicy.h"

namespace Unbound {
class ArrangementStore
{
public:
    ArrangementStore() : path(qEnvironmentVariable("UNBOUND_ARRANGEMENTS_FILE",
        settingsProfilePath("Arrangements.json")))
    {
        QFile file(path);
        if (!file.exists()) return;
        QJsonParseError error;
        if (!file.open(QIODevice::ReadOnly)) { failure = "Cannot read saved arrangements."; return; }
        const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject() ||
            doc.object()["version"].toInt() != 1 || !doc.object()["arrangements"].isArray()) {
            failure = "Saved arrangements are invalid or from an unsupported version."; return;
        }
        entries = doc.object()["arrangements"].toArray();
        QSet<QString> ids, names;
        for (const auto &value : entries) {
            const auto entry = value.toObject();
            const auto id = entry["id"].toString(), name = workspacePage(entry)+QLatin1Char('\n')+entry["name"].toString().trimmed().toCaseFolded();
            if (id.isEmpty() || entry["name"].toString().trimmed().isEmpty() || ids.contains(id) || names.contains(name) || !entry["workspace"].isObject()) {
                failure = "Saved arrangements contain invalid or duplicate entries."; return;
            }
            ids.insert(id); names.insert(name);
        }
    }
    QJsonArray list() const { return entries; }
    QJsonArray list(const QString &page) const
    { QJsonArray result; for (const auto &entry : entries) if (workspacePage(entry.toObject()) == page) result.append(entry); return result; }
    QJsonArray visibleList(const QString &page = {}) const {
        QJsonArray result;for(const auto &value:entries){const auto entry=value.toObject();
            if(!entry["hidden"].toBool() && (page.isEmpty() || workspacePage(entry)==page))result.append(entry);
        }return result;
    }
    bool hasHidden() const {for(const auto &entry:entries)if(entry.toObject()["hidden"].toBool())return true;return false;}
    QString revealHidden() {
        auto next=entries;for(int i=0;i<next.size();++i){auto entry=next[i].toObject();entry.remove("hidden");next[i]=entry;}
        return write(next);
    }
    QString error() const { return failure; }
    QJsonObject find(const QString &id) const
    { for (const auto &entry : entries) if (entry.toObject()["id"] == id) return entry.toObject(); return {}; }
    QString create(const QString &name, const QJsonObject &workspace, QString &id)
    {
        const QString page = workspace.value("page").toString("edit");
        const QString problem = validName(name,{},page);
        if (!problem.isEmpty()) return problem;
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        auto next = entries;
        next.append(QJsonObject{{"id", id}, {"name", name.trimmed()}, {"page",page}, {"workspace", workspace}});
        return write(next);
    }
    QString update(const QString &id, const QJsonObject &workspace)
    { return change(id, QString(), workspace, false); }
    QString rename(const QString &id, const QString &name)
    {
        const auto problem = validName(name, id,workspacePage(find(id)));
        return problem.isEmpty() ? change(id, name.trimmed(), {}, false) : problem;
    }
    QString remove(const QString &id) { return change(id, {}, {}, true); }
    QString reorder(const QStringList &order)
    {
        if (order.size() != entries.size()) return "Arrangement order does not match saved presets.";
        QJsonArray next; QSet<QString> seen;
        for (const auto &id : order) {
            const auto entry = find(id);
            if (seen.contains(id) || entry.isEmpty()) return "Arrangement order contains missing or duplicate presets.";
            seen.insert(id); next.append(entry);
        }
        return write(next);
    }
    QString reorderPage(const QString &page,const QStringList &order)
    {
        if (order.size() != visibleList(page).size()) return "Arrangement order does not match this page's saved presets.";
        for (const auto &id : order) if (workspacePage(find(id)) != page || find(id)["hidden"].toBool()) return "Arrangement belongs to another page or is hidden.";
        QStringList all; int next = 0;
        for (const auto &entry : entries) all.append(workspacePage(entry.toObject()) == page && !entry.toObject()["hidden"].toBool() ? order[next++] : entry.toObject()["id"].toString());
        return reorder(all);
    }
private:
    QString validName(const QString &name, const QString &except = {}, const QString &page = "edit") const
    {
        if (name.trimmed().isEmpty()) return "Enter an arrangement name.";
        if (name.trimmed().compare("Resolve Layout",Qt::CaseInsensitive)==0) return "Resolve Layout is reserved. Choose a name for your workspace.";
        for (auto value : entries) {
            auto entry = value.toObject();
            if (entry["id"] != except && workspacePage(entry) == page && entry["name"].toString().compare(name.trimmed(), Qt::CaseInsensitive) == 0)
                return "An arrangement already has that name.";
        }
        return {};
    }
    QString change(const QString &id, const QString &name, const QJsonObject &workspace, bool erase)
    {
        if (WorkspacePresetPolicy::builtIn(id)) return "Resolve Layout is read-only. Save as a new workspace instead.";
        auto next = entries;
        for (int i = 0; i < next.size(); ++i) {
            auto entry = next[i].toObject();
            if (entry["id"] != id) continue;
            if (!workspace.isEmpty() && workspace.value("page").toString("edit") != workspacePage(entry))
                return "A workspace cannot be updated from a different Resolve page.";
            if (erase) next.removeAt(i);
            else {
                if (!name.isNull()) entry["name"] = name;
                if (!workspace.isEmpty()) entry["workspace"] = workspace;
                next[i] = entry;
            }
            return write(next);
        }
        return "Arrangement no longer exists.";
    }
    QString write(const QJsonArray &next)
    {
        // Preserve invalid files for recovery instead of overwriting them.
        if (!failure.isEmpty()) return failure;
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) return "Cannot create arrangements folder.";
        QSaveFile file(path);
        const auto bytes = QJsonDocument(QJsonObject{{"version", 1}, {"arrangements", next}}).toJson();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            return "Cannot save arrangements.";
        entries = next; return {};
    }
    QString path, failure;
    QJsonArray entries;
};
}
