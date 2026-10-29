// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QList>
#include <QRect>
#include <limits>
namespace Unbound {
// Logical screen coordinates only. Native window ownership and Qt discovery are
// adapter responsibilities. No two-display assumption or desktop-origin clamp.
struct WorkspaceDisplay { QString name,serial,manufacturer,model; QRect available; bool primary=false; };
struct DisplayPlacement {
    static QJsonObject rect(QRect r) {return {{"x",r.x()},{"y",r.y()},{"width",r.width()},{"height",r.height()}};}
    static QRect rect(QJsonObject r) {return {r["x"].toInt(),r["y"].toInt(),r["width"].toInt(),r["height"].toInt()};}
    static int match(const QJsonObject &record,const QList<WorkspaceDisplay> &screens) {
        const auto id=record["display"].toObject();if(id.isEmpty())return -1;
        int selected=-1; qint64 distance=std::numeric_limits<qint64>::max();
        const auto saved=rect(id["available"].toObject()).center();
        for(int i=0;i<screens.size();++i){const auto &s=screens[i];
            const bool matches=!id["serial"].toString().isEmpty() ? s.serial==id["serial"].toString() && s.manufacturer==id["manufacturer"].toString() :
                s.name==id["name"].toString() && s.model==id["model"].toString() && s.manufacturer==id["manufacturer"].toString();
            if(!matches)continue;
            const auto delta=s.available.center()-saved;const qint64 d=qint64(delta.x())*delta.x()+qint64(delta.y())*delta.y();
            if(d<distance){selected=i;distance=d;}
        }return selected;
    }
    static QRect reachable(QRect r,QRect area) {
        if(!r.isValid()||!area.isValid())return r;
        r.setSize(r.size().boundedTo(area.size()));
        r.moveLeft(qBound(area.left(),r.left(),area.right()-r.width()+1));
        r.moveTop(qBound(area.top(),r.top(),area.bottom()-r.height()+1));return r;
    }
    static QRect resolve(const QJsonObject &record,const QList<WorkspaceDisplay> &screens) {
        auto r=rect(record);if(!r.isValid()||screens.isEmpty())return r;
        const int target=match(record,screens);
        if(target>=0){r=rect(record["display_relative"].toObject());if(!r.isValid())r=rect(record);else r.translate(screens[target].available.topLeft());return rect(record["display"].toObject()["available"].toObject()).size()==screens[target].available.size() ? r : reachable(r,screens[target].available);}
        // Legacy records retain their absolute position on a still-present screen.
        int best=-1; qint64 overlap=0;
        for(int i=0;i<screens.size();++i){const auto part=r.intersected(screens[i].available);const qint64 area=qint64(part.width())*part.height();if(part.isValid()&&area>overlap){best=i;overlap=area;}}
        if(best<0){best=0;for(int i=0;i<screens.size();++i)if(screens[i].primary){best=i;break;}}
        return reachable(r,screens[best].available);
    }
    static QJsonObject capture(QRect geometry,const QList<WorkspaceDisplay> &screens,const QJsonObject &previous={}) {
        // A missing-display fallback is temporary. Repeated auto-saves must not
        // destroy the original placement; an intentional move creates a new one.
        if(previous.contains("display")&&match(previous,screens)<0&&(previous.contains("display_fallback")?rect(previous["display_fallback"].toObject()):resolve(previous,screens))==geometry)return previous;
        auto out=rect(geometry);if(!geometry.isValid()||screens.isEmpty())return out;
        int best=0;qint64 overlap=-1;
        for(int i=0;i<screens.size();++i){const auto part=geometry.intersected(screens[i].available);const qint64 area=part.isValid()?qint64(part.width())*part.height():0;if(area>overlap){overlap=area;best=i;}}
        const auto &s=screens[best];out["display"]=QJsonObject{{"name",s.name},{"serial",s.serial},{"manufacturer",s.manufacturer},{"model",s.model},{"available",rect(s.available)}};
        out["display_relative"]=rect(geometry.translated(-s.available.topLeft()));return out;
    }
};
}
