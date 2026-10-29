// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringList>

namespace Unbound {
inline bool groupTreeNames(const QJsonObject &node, QStringList &names, int depth = 0)
{
    if (depth > 24 || names.size() > 32) return false;
    if (node.contains("panel")) {
        const auto name = node["panel"].toString();
        if (name.isEmpty() || names.contains(name)) return false;
        names.append(name); return true;
    }
    const auto children = node["children"].toArray();
    if ((node["orientation"] != "horizontal" && node["orientation"] != "vertical" && node["orientation"] != "tabs") || children.size() < 2 || children.size() > 32) return false;
    for (auto child : children) if (!groupTreeNames(child.toObject(), names, depth + 1)) return false;
    return true;
}
// A transient target key identifies a retained subtree by its ordered leaves.
// It is never persisted as a panel ID or shown as a user-facing title.
inline QString groupSectionTarget(const QJsonObject &node) {
    if(node.contains("panel"))return {};
    QStringList names;if(!groupTreeNames(node,names))return {};
    return "@section:"+QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(names)).toJson(QJsonDocument::Compact));
}
inline bool groupTreeHasTarget(const QJsonObject &node,const QString &target) {
    if(target.isEmpty())return false;
    if(node.contains("panel"))return node["panel"].toString()==target;
    if(groupSectionTarget(node)==target)return true;
    for(auto child:node["children"].toArray())if(groupTreeHasTarget(child.toObject(),target))return true;
    return false;
}
inline QJsonObject splitGroupTree(const QJsonObject &a, const QJsonObject &b, Qt::Orientation orientation, bool before = false)
{
    return {{"orientation", orientation == Qt::Horizontal ? "horizontal" : orientation == Qt::Vertical ? "vertical" : "tabs"},
        {"children", before ? QJsonArray{b, a} : QJsonArray{a, b}}, {"sizes", QJsonArray{1, 1}}};
}
inline QJsonObject insertGroupTree(QJsonObject node, const QJsonObject &addition, const QString &target, Qt::Orientation orientation, bool before)
{
    if (orientation == Qt::Orientation(0) && node.value("orientation") == "tabs") {
        auto children = node["children"].toArray();
        bool matches = target.isEmpty();
        for (auto child : children) matches |= child.toObject().value("panel") == target;
        if (matches) {
            const int selected = before ? 0 : children.size();
            if (before) children.prepend(addition); else children.append(addition);
            node["children"] = children; node["selected"] = selected; return node;
        }
    }
    if (target.isEmpty() || node.value("panel") == target || (!node.contains("panel") && groupSectionTarget(node)==target)) {
        auto result=splitGroupTree(node,addition,orientation,before);
        if (orientation==Qt::Orientation(0)) result["selected"]=before ? 0 : 1;
        return result;
    }
    if (node.contains("panel")) return node;
    auto children = node["children"].toArray();
    for (int i = 0; i < children.size(); ++i) children[i] = insertGroupTree(children[i].toObject(), addition, target, orientation, before);
    node["children"] = children; return node;
}
inline QJsonObject removeGroupLeaf(QJsonObject node, const QString &name)
{
    if (node.contains("panel")) return node["panel"] == name ? QJsonObject() : node;
    QJsonArray children, sizes;
    const auto old = node["children"].toArray(), oldSizes = node["sizes"].toArray();
    const int previousSelection=qBound(0,node.value("selected").toInt(),qMax(0,old.size()-1));
    int retainedSelection=-1,survivingBefore=0;
    for (int i = 0; i < old.size(); ++i) {
        const auto child = removeGroupLeaf(old[i].toObject(), name);
        if (!child.isEmpty()) {
            if (i==previousSelection) retainedSelection=children.size();
            if (i<previousSelection) ++survivingBefore;
            children.append(child); sizes.append(i < oldSizes.size() ? oldSizes[i] : QJsonValue(1));
        }
    }
    if (children.isEmpty()) return {};
    if (children.size() == 1) return children[0].toObject();
    node["children"] = children; node["sizes"] = sizes;
    if (node["orientation"]=="tabs") node["selected"]=retainedSelection>=0 ? retainedSelection : qMin(survivingBefore,children.size()-1);
    return node;
}
}
