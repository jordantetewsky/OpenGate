// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPointer>
#include <QSet>
#include "clipfillregionmap.h"
namespace Unbound {
// One native root view owns weak clip/color bindings and their scene geometry.
// No painter, project writer, command handler or second scene is owned here.
class TimelineClipColorBindings : public QObject {
public:
    static TimelineClipColorBindings *find(QGraphicsView *root) {
        return root?dynamic_cast<TimelineClipColorBindings *>(root->findChild<QObject *>(QStringLiteral("openGateClipColorBindings"),Qt::FindDirectChildrenOnly)):nullptr;
    }
    static TimelineClipColorBindings *ensure(QGraphicsView *root) {
        if(auto existing=find(root))return existing;
        return new TimelineClipColorBindings(root);
    }
    bool apply(QGraphicsView *surface,QGraphicsObject *clip,QColor color) {
        if(!surface || !clip || clip->scene()!=surface->scene() || clip->graphicsEffect() ||
           surface->graphicsEffect() || surface->viewport()->graphicsEffect())return false;
        for(int i=entries.size()-1;i>=0;--i)if(!entries[i].clip || !entries[i].surface)entries.removeAt(i);
        for(auto &entry:entries)if(entry.clip==clip){entry.color=color;changed();return true;}
        entries.append({clip,surface,color});changed();return true;
    }
    void clear(const QSet<QGraphicsObject *> &selected={}) {
        for(int i=entries.size()-1;i>=0;--i)if(selected.isEmpty() || !entries[i].clip || selected.contains(entries[i].clip))entries.removeAt(i);
        changed();
    }
    QVector<ClipFillRegion> regions() const {
        QVector<ClipFillRegion> result;
        for(const auto &entry:entries)if(entry.clip && entry.surface && entry.clip->isVisible() && entry.clip->scene()==entry.surface->scene()) {
            const auto full=entry.clip->sceneBoundingRect();
            const auto exposed=full.intersected(entry.surface->mapToScene(entry.surface->viewport()->rect()).boundingRect());
            if(exposed.isEmpty())continue;
            const auto bounds=inRootScene(entry,full),visible=inRootScene(entry,exposed);
            if(!bounds.isEmpty() && !visible.isEmpty())result.append({bounds,entry.color,visible});
        }
        return result;
    }
    quint64 revision() const{return version;}
private:
    struct Entry {QPointer<QGraphicsObject> clip;QPointer<QGraphicsView> surface;QColor color;};
    QPointer<QGraphicsView> root;QVector<Entry> entries;quint64 version=0;
    explicit TimelineClipColorBindings(QGraphicsView *root):QObject(root),root(root){setObjectName(QStringLiteral("openGateClipColorBindings"));}
    void changed(){++version;if(root){if(root->scene())root->scene()->invalidate();root->viewport()->update();}}
    QRectF inRootScene(const Entry &entry,QRectF bounds) const {
        if(!root)return {};
        auto surface=entry.surface.data();
        for(int depth=0;surface && depth<16;++depth) {
            if(surface->scene()==root->scene())return bounds;
            bounds=surface->viewportTransform().mapRect(bounds);
            QWidget *host=surface;while(host && !host->graphicsProxyWidget())host=host->parentWidget();
            if(!host)return {};auto proxy=host->graphicsProxyWidget();
            bounds.translate(surface->viewport()->mapTo(host,QPoint()));bounds=proxy->mapRectToScene(bounds);
            if(proxy->scene()==root->scene())return bounds;
            const auto parents=proxy->scene()?proxy->scene()->views():QList<QGraphicsView *>();
            if(parents.size()!=1)return {};surface=parents.front();
        }
        return {};
    }
};
}
