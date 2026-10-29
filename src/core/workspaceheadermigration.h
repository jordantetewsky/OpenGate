// SPDX-License-Identifier: MIT
#pragma once
#include "panelgroup.h"
namespace Unbound {
// Convert only the legacy toolbar leaf. Preserve every other panel, subtree,
// group frame and unknown settings field. The original settings are backed up
// by installation; arrangement records themselves are never rewritten here.
inline QJsonObject migrateAttachedToolbarLeaf(QJsonObject value,const QString &id) {
    auto panels=value["panels"].toObject();auto toolbar=panels[id].toObject();
    if(id=="unboundWorkspaceToolbar"&&!value.contains("header"))value["header"]=QJsonObject{{"model","pinned-top-v1"},{"visible",!toolbar["hidden"].toBool()}};
    toolbar["floating"]=false;panels[id]=toolbar;
    auto footer=value["footer"].toObject();auto surfacePins=footer["surfaces"].toObject();
    QJsonArray groups;
    for(auto entry:value["groups"].toArray()) {
        auto group=entry.toObject();const auto tree=PanelGroup::savedTree(group);QStringList names;
        if(!groupTreeNames(tree,names) || !names.contains(id)){groups.append(group);continue;}
        auto oldNames=names;oldNames.sort();const auto oldSurface="unboundFloating_group_"+oldNames.join('_');
        const auto remaining=removeGroupLeaf(tree,id);names.clear();
        if(!remaining.isEmpty() && groupTreeNames(remaining,names)) {
            auto sorted=names;sorted.sort();const auto newSurface=names.size()>1 ? "unboundFloating_group_"+sorted.join('_') : "unboundFloating_"+names.first();
            if(surfacePins.contains(oldSurface))surfacePins[newSurface]=surfacePins[oldSurface];
            if(names.size()>1){group["tree"]=remaining;group["panels"]=QJsonArray::fromStringList(names);groups.append(group);}
            else {
                auto panel=panels[names.first()].toObject();const auto geometry=WorkspaceSettings::rectangle(group);
                if(geometry.isValid()){const auto rect=WorkspaceSettings::rectangle(geometry);for(auto it=rect.begin();it!=rect.end();++it)panel[it.key()]=it.value();}
                panel["floating"]=true;panels[names.first()]=panel;
            }
        }
    }
    footer["surfaces"]=surfacePins;value["footer"]=footer;
    value["groups"]=groups;value["panels"]=panels;return value;
}
inline QJsonObject migrateWorkspaceHeader(QJsonObject value) {
    value=migrateAttachedToolbarLeaf(value,"unboundWorkspaceToolbar");
    return migrateAttachedToolbarLeaf(value,"frameEditToolbarTop");
}
}
