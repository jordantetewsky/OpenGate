// SPDX-License-Identifier: MIT
#pragma once
#include <QSet>
#include <limits>
#include "grouplayout.h"
namespace Unbound {
inline QString nativeRemainderId() { return QStringLiteral("unbound.native-remainder"); }
struct NativeArrangementPlan {
    QString page;
    QString anchor;
    bool managed=false;
    QJsonObject tree;
    QSet<QString> hidden;
    QJsonObject json() const {
        QStringList names; groupTreeNames(tree,names); QJsonObject panels;
        for (const auto &id : names) if (id!=nativeRemainderId()) panels[id]=QJsonObject{{"hidden",hidden.contains(id)}};
        QJsonObject result{{"version",1},{"kind",managed ? "managed" : "native"},{"page",page},{"tree",tree},{"panels",panels}};
        if (!anchor.isEmpty()) result["anchor"]=anchor;
        return result;
    }
    static QString parse(const QJsonObject &value,const QString &expectedPage,const QSet<QString> &available,NativeArrangementPlan &output) {
        if (value["version"]!=1 || (value["kind"]!="native" && value["kind"]!="managed") || expectedPage.isEmpty() || value["page"]!=expectedPage)
            return QStringLiteral("This native arrangement belongs to another page or format.");
        const auto tree=value["tree"].toObject(); QStringList names;
        const auto anchor=value["anchor"].toString(); const bool scoped=!anchor.isEmpty(); const bool managed=value["kind"]=="managed";
        if ((value.contains("anchor") && (!value["anchor"].isString() || anchor.isEmpty() || !available.contains(anchor))) ||
            !groupTreeNames(tree,names) || names.size()<((scoped || managed) ? 1 : 2) || names.size()>32 ||
            (managed ? (scoped || names.contains(nativeRemainderId())) : names.contains(nativeRemainderId())==scoped) || !geometryValid(tree))
            return QStringLiteral("The saved native arrangement tree is invalid.");
        if (!value["panels"].isObject()) return QStringLiteral("The saved native panel states are missing.");
        const auto panels=value["panels"].toObject();
        if (panels.size()!=names.size()-((scoped || managed) ? 0 : 1)) return QStringLiteral("The saved native panel states do not match the arrangement.");
        NativeArrangementPlan candidate; candidate.page=expectedPage; candidate.tree=tree; candidate.anchor=anchor; candidate.managed=managed;
        for (const auto &id : names) if (id!=nativeRemainderId()) {
            if (!available.contains(id) || !panels[id].isObject() || !panels[id].toObject()["hidden"].isBool())
                return QStringLiteral("A saved native panel is unavailable or has invalid visibility.");
            if (panels[id].toObject()["hidden"].toBool()) candidate.hidden.insert(id);
        }
        output=candidate; return {};
    }
private:
    static bool boundedInteger(const QJsonValue &value,int minimum,int maximum) {
        return value.isDouble() && value.toDouble()>=minimum && value.toDouble()<=maximum && value.toDouble()==value.toInt();
    }
    static bool geometryValid(const QJsonObject &node) {
        if (node.contains("panel")) return true;
        const auto children=node["children"].toArray();
        if (node["orientation"]=="tabs") {
            if (node.contains("selected") && !boundedInteger(node["selected"],0,children.size()-1)) return false;
        } else if (node.contains("sizes")) {
            if (!node["sizes"].isArray() || node["sizes"].toArray().size()!=children.size()) return false;
            for (const auto &size : node["sizes"].toArray())
                if (!boundedInteger(size,1,std::numeric_limits<int>::max()/32)) return false;
        }
        for (const auto &child : children) if (!geometryValid(child.toObject())) return false;
        return true;
    }
};
}
