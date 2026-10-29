// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QSplitter>
#include <QPointer>
#include <QSet>
#include <QTabWidget>
#include <functional>
#include "tabtearoff.h"
#include "grouplayout.h"
#include "panellayouttitles.h"
namespace Unbound {
struct PanelLayoutCallbacks {
    std::function<QWidget *(const QString &)> leaf;
    std::function<QString(const QString &)> title;
    std::function<void(QWidget *,QPoint)> tearOff;
    std::function<void()> changed;
};
// On a failed render, delete only containers we created, never native leaves.
inline void abandonPanelLayoutSurface(QWidget *surface,QWidget *parent,const QList<QPointer<QWidget>> &leaves) {
    for (const auto &leaf : leaves) if (leaf && surface->isAncestorOf(leaf)) leaf->setParent(parent);
    delete surface;
}
inline bool validatePanelLayoutSurface(const QJsonObject &node,const PanelLayoutCallbacks &callbacks,
    QSet<QString> &ids,QList<QPointer<QWidget>> &leaves,int depth=0) {
    if (depth>24 || ids.size()>32) return false;
    if (node.contains("panel")) {
        const auto id=node["panel"].toString();
        auto leaf=callbacks.leaf ? callbacks.leaf(id) : nullptr;
        if (id.isEmpty() || ids.contains(id) || !leaf || leaves.contains(leaf)) return false;
        ids.insert(id); leaves.append(leaf); return ids.size()<=32;
    }
    if (node["orientation"]!="tabs" && node["orientation"]!="horizontal" && node["orientation"]!="vertical") return false;
    const auto children=node["children"].toArray(); if (children.size()<2 || children.size()>32) return false;
    for (const auto &child : children) if (!validatePanelLayoutSurface(child.toObject(),callbacks,ids,leaves,depth+1)) return false;
    return true;
}
// A layout surface has no OS window, pin, native restoration slot or session
// ownership. Both a floating host and an embedded native host can render it.
inline QWidget *buildPanelLayoutSurfaceNode(const QJsonObject &node,QWidget *parent,QObject *owner,
    const PanelLayoutCallbacks &callbacks,const QList<QPointer<QWidget>> &leaves,int depth=0)
{
    if (depth>24) return nullptr;
    if (node.contains("panel")) return callbacks.leaf ? callbacks.leaf(node["panel"].toString()) : nullptr;
    const auto children=node["children"].toArray();
    if (children.size()<2 || children.size()>32) return nullptr;
    if (node["orientation"]=="tabs") {
        auto tabs=new QTabWidget(parent); tabs->setObjectName("unboundPanelTabs");
        tabs->setProperty("openGateDockSection",groupSectionTarget(node));
        tabs->setDocumentMode(true); tabs->setMovable(false);
        tabs->setElideMode(Qt::ElideRight);tabs->setUsesScrollButtons(true);
        for (const auto &child : children) {
            auto widget=buildPanelLayoutSurfaceNode(child.toObject(),parent,owner,callbacks,leaves,depth+1);
            if (!widget) { abandonPanelLayoutSurface(tabs,parent,leaves); return nullptr; }
            const auto title=panelLayoutTitle(child.toObject(),callbacks.title);
            const int index=tabs->addTab(widget,title);tabs->setTabToolTip(index,title);
        }
        if (callbacks.tearOff) new TabTearOff(tabs,tabs,callbacks.tearOff);
        tabs->setCurrentIndex(qBound(0,node["selected"].toInt(),tabs->count()-1));
        QObject::connect(tabs,&QTabWidget::currentChanged,owner,[callbacks] { if (callbacks.changed) callbacks.changed(); });
        return tabs;
    }
    if (node["orientation"]!="horizontal" && node["orientation"]!="vertical") return nullptr;
    auto splitter=new QSplitter(node["orientation"]=="vertical" ? Qt::Vertical : Qt::Horizontal,parent);
    splitter->setChildrenCollapsible(false); splitter->setHandleWidth(6);
    splitter->setProperty("openGateDockSection",groupSectionTarget(node));
    for (const auto &child : children) {
        auto widget=buildPanelLayoutSurfaceNode(child.toObject(),parent,owner,callbacks,leaves,depth+1);
        if (!widget) { abandonPanelLayoutSurface(splitter,parent,leaves); return nullptr; }
        splitter->addWidget(widget);
    }
    splitter->setProperty("savedSizes",node["sizes"].toArray().toVariantList());
    QList<int> initialSizes;
    for (const auto &size : node["sizes"].toArray()) initialSizes.append(qMax(1,size.toInt()));
    if (initialSizes.size()!=splitter->count()) {
        initialSizes.clear(); for (int i=0;i<splitter->count();++i) initialSizes.append(1);
    }
    splitter->setProperty("unboundVisibleSizes",QVariant::fromValue(initialSizes));
    QObject::connect(splitter,&QSplitter::splitterMoved,owner,[callbacks] { if (callbacks.changed) callbacks.changed(); });
    return splitter;
}
inline QWidget *buildPanelLayoutSurface(const QJsonObject &node,QWidget *parent,QObject *owner,const PanelLayoutCallbacks &callbacks) {
    if (!parent || !owner) return nullptr;
    QSet<QString> ids; QList<QPointer<QWidget>> leaves;
    if (!validatePanelLayoutSurface(node,callbacks,ids,leaves)) return nullptr;
    return buildPanelLayoutSurfaceNode(node,parent,owner,callbacks,leaves);
}

}
