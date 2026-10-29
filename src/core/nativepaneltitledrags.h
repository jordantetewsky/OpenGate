// SPDX-License-Identifier: MIT
#pragma once
#include <QLabel>
#include <QSet>
#include "paneltitlegesture.h"
#include "diagnostics.h"
namespace Unbound {
// Existing title labels/buttons only. Never put an input overlay over a viewer
// or timeline body, selectable label, text field or native options control.
class NativePanelTitleDrags : public QObject {
public:
    explicit NativePanelTitleDrags(QObject *owner):QObject(owner){}
    void wire(QWidget *surface,const QString &id,const QString &title,std::function<void(QPoint)> move) {
        if(!surface)return;
        for(auto widget:surface->findChildren<QWidget *>()) {
            if(widget->property("openGatePanelTitleGesture").toBool())continue;
            QString text;
            const bool mixerTitle=id=="frameEditAudioMeter" && widget->objectName()=="lblMixerMeter" &&
                widget->parentWidget() && widget->parentWidget()->objectName()=="frameMixerMeterTitle";
            // UiElidedLabel may inherit QLabel while painting text held outside
            // QLabel::text(). Its stable native identity takes precedence.
            if(mixerTitle)text=title;
            else if(auto label=qobject_cast<QLabel *>(widget)) {
                if(label->textInteractionFlags()!=Qt::NoTextInteraction)continue;
                text=label->text();
            } else if(auto button=qobject_cast<QAbstractButton *>(widget))text=button->text();
            else continue;
            text.remove('&');if(text.trimmed()!=title || wired.contains(widget))continue;
            // Match only the shallow native title row, not arbitrary body text.
            if(widget->mapTo(surface,QPoint()).y()>80)continue;
            wired.insert(widget);connect(widget,&QObject::destroyed,this,[this,widget]{wired.remove(widget);});
            new PanelTitleGesture(widget,this,[id,move](QPoint point){
                logEvent("panel.title.drag",{{"panel",id},{"x",point.x()},{"y",point.y()}});if(move)move(point);
            },mixerTitle?title:QString());
            if(mixerTitle)widget->setToolTip(title+"; drag to move or detach");
            logEvent("panel.title.wired",{{"panel",id},{"title",widget->objectName()},{"surface",surface->objectName()}});
        }
    }
private:QSet<QWidget *> wired;
};
}
