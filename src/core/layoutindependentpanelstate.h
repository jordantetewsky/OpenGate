// SPDX-License-Identifier: MIT
#pragma once
#include "panelgroup.h"
#include "nativearrangementplan.h"

namespace Unbound {
// Projects an incoming workspace around panels whose live placement is independent
// of workspace selection. Owns no state; callers supply the existing saved records.
class LayoutIndependentPanelState {
public:
    static QJsonObject preserve(QJsonObject workspace,const QJsonObject &retained) {
        auto panels=workspace["panels"].toObject();
        QJsonArray groups;
        for(const auto &entry:workspace["groups"].toArray()) {
            auto group=entry.toObject();const auto original=PanelGroup::savedTree(group);
            const auto tree=without(original,retained);
            if(tree==original){groups.append(group);continue;}
            retainGroup(group,tree,panels,groups);
        }
        auto native=workspace["native_arrangement"].toObject();
        if(!native.isEmpty()) {
            const auto tree=without(native["tree"].toObject(),retained);
            auto states=native["panels"].toObject();
            for(auto it=retained.begin();it!=retained.end();++it)states.remove(it.key());
            if(states.isEmpty())native={};
            else if(retained.contains(native["anchor"].toString())) {
                // A scoped arrangement cannot retain an independent panel as
                // its native anchor. Keep the other members together floating.
                retainGroup({},tree,panels,groups);native={};
            } else {native["tree"]=tree;native["panels"]=states;}
        }
        for(auto it=retained.begin();it!=retained.end();++it)panels[it.key()]=it.value();
        workspace["panels"]=panels;workspace["groups"]=groups;
        workspace["native_arrangement"]=native;
        return workspace;
    }
private:
    static QJsonObject without(QJsonObject tree,const QJsonObject &retained) {
        QStringList names;
        if(!groupTreeNames(tree,names))return tree;
        for(auto it=retained.begin();it!=retained.end();++it)
            if(names.contains(it.key()))tree=removeGroupLeaf(tree,it.key());
        return tree;
    }
    static void retainGroup(QJsonObject group,const QJsonObject &tree,QJsonObject &panels,QJsonArray &groups) {
        QStringList names;
        if(tree.isEmpty() || !groupTreeNames(tree,names))return;
        if(names.size()>1) {
            group["tree"]=tree;group["panels"]=QJsonArray::fromStringList(names);groups.append(group);
        } else {
            auto state=panels[names.first()].toObject();
            const auto frame=WorkspaceSettings::rectangle(group);
            if(frame.isValid()) {
                const auto geometry=WorkspaceSettings::rectangle(frame);
                for(auto it=geometry.begin();it!=geometry.end();++it)state[it.key()]=it.value();
            }
            state["floating"]=true;panels[names.first()]=state;
        }
    }
};
}
