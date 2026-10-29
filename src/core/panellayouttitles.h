// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>
namespace Unbound {
inline QString panelLayoutTitle(const QJsonObject &node,const std::function<QString(const QString &)> &title,int depth=0) {
    if(depth>24)return {};
    if(node.contains("panel"))return title?title(node["panel"].toString()):node["panel"].toString();
    QStringList names;
    for(auto child:node["children"].toArray()) {
        const auto name=panelLayoutTitle(child.toObject(),title,depth+1);
        if(!name.isEmpty())names.append(name);
    }
    return names.join(" + ");
}
}
