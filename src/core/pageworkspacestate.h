// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
namespace Unbound {
// Page ownership/persistence only. Native widget discovery and transitions stay
// with their existing controllers. A future mixed-page workspace needs a separate
// source-page lifecycle contract; it must not bypass this page boundary.
class PageWorkspaceState {
public:
    static QJsonObject snapshot(QJsonObject value) {
        value.remove("page_workspaces");
        return value;
    }
    static QJsonObject view(const QJsonObject &root,const QString &page) {
        const auto entry=root["page_workspaces"].toObject()[page].toObject();
        const auto saved=entry["last_view"].toObject();
        if(entry["version"]==1 && saved["page"]==page)return snapshot(saved);
        if(page=="edit" && !root.isEmpty()) {
            auto legacy=snapshot(root);
            if(legacy.value("page").toString("edit")!="edit")return {};
            legacy["page"]="edit";return legacy;
        }
        return {};
    }
    static QString mode(const QJsonObject &value) {
        const auto explicitMode=value["layout_mode"].toString();
        if(explicitMode=="native" || explicitMode=="managed")return explicitMode;
        if(!value["groups"].toArray().isEmpty() || !value["native_arrangement"].toObject().isEmpty())return "managed";
        const auto panels=value["panels"].toObject();
        for(auto it=panels.begin();it!=panels.end();++it)if(it.value().toObject()["floating"].toBool() || it.value().toObject().contains("native_hidden"))return "managed";
        return "native";
    }
    static QString record(QJsonObject &root,const QString &page,QJsonObject value) {
        value=snapshot(value);
        if(page.isEmpty() || value.value("page").toString("edit")!=page)
            return "Workspace snapshot belongs to a different page.";
        const auto native=value["native_arrangement"].toObject();
        if(!native.isEmpty() && native.value("page").toString("edit")!=page)
            return "Native arrangement belongs to a different page.";
        auto pages=root["page_workspaces"].toObject();auto entry=pages[page].toObject();
        if(!entry.isEmpty() && entry["version"]!=1)return "Unsupported page workspace state version.";
        // Preserve the last complete custom view before explicit Native return.
        // This is independent of the selected preset: unsaved edits matter too.
        const auto previous=view(root,page);
        if(entry["last_managed_view"].toObject().isEmpty() && !previous.isEmpty() && mode(previous)=="managed")
            entry["last_managed_view"]=previous;
        value["page"]=page;const auto currentMode=mode(value);value["layout_mode"]=currentMode;
        entry["version"]=1;entry["mode"]=currentMode;entry["last_view"]=value;
        entry["selected_arrangement"]=value["selected_arrangement"].toString();
        if(currentMode=="managed")entry["last_managed_view"]=value;
        pages[page]=entry;
        // Keep the legacy Edit projection readable by the established Edit
        // owner. Other pages never replace those top-level Edit fields.
        if(page=="edit")root=value;
        root["page_workspaces"]=pages;return {};
    }
    static QJsonObject managedView(const QJsonObject &root,const QString &page) {
        const auto entry=root["page_workspaces"].toObject()[page].toObject();
        const auto saved=entry["last_managed_view"].toObject();
        return entry["version"]==1 && saved["page"]==page ? snapshot(saved) : QJsonObject{};
    }
};
}
