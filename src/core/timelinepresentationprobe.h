// SPDX-License-Identifier: MIT
#pragma once
#include <QEvent>
#include <QLayout>
#include <QPointer>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Temporary read-only transition evidence. Does not own presentation or input.
class TimelinePresentationProbe : public QObject {
public:
    explicit TimelinePresentationProbe(QObject *owner):QObject(owner) {}
    void observe(QWidget *widget,QEvent::Type type) {
        if(!widget || captures>=256 || !relevant(widget->objectName()))return;
        if(type!=QEvent::Show && type!=QEvent::Hide && type!=QEvent::ParentChange &&
           type!=QEvent::Resize && type!=QEvent::LayoutRequest)return;
        const auto before=snapshot(widget);
        const auto sequence=++captures;
        logEvent("timeline.presentation.transition",{{"sequence",sequence},{"event_type",int(type)},
            {"phase","before-handler"},{"state",before},{"read_only",true}});
        QPointer<QWidget> alive=widget;
        QTimer::singleShot(0,this,[alive,sequence] {
            logEvent("timeline.presentation.transition",{{"sequence",sequence},{"phase","next-turn"},
                {"state",alive?snapshot(alive):QJsonObject{{"destroyed",true}}},{"read_only",true}});
        });
    }
private:
    int captures=0;
    static bool relevant(const QString &name) {
        return name=="frameEditToolbarTop" || name=="frameConformTimelines" ||
            name=="unboundNativePanelPosition_frameEditToolbarTop" ||
            name=="unboundPanelOverflow_frameEditToolbarTop";
    }
    static QJsonObject snapshot(QWidget *widget) {
        QJsonArray ancestry,children;
        for(auto p=widget;p && ancestry.size()<12;p=p->parentWidget()) {
            auto state=widgetState(p,false);
            state["maximum_height"]=p->maximumHeight();
            state["layout_class"]=p->layout()?p->layout()->metaObject()->className():"";
            state["layout_index"]=p->parentWidget() && p->parentWidget()->layout()?
                p->parentWidget()->layout()->indexOf(p):-1;
            ancestry.append(state);
        }
        for(auto child:widget->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly)) {
            if(children.size()>=24)break;
            children.append(widgetState(child,false));
        }
        return {{"ancestry",ancestry},{"children",children}};
    }
};
}
