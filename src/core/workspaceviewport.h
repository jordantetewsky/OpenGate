// SPDX-License-Identifier: MIT
#pragma once
#include <QLayout>
#include <QWidget>
namespace Unbound {
// The workspace already lays out its visible bands. Use its actual content
// rectangle rather than reconstructing screen-wide gates or native constraints.
inline QRect workspaceViewportBounds(QWidget *owner,QWidget *nativeBody=nullptr) {
    if(!owner || !owner->isVisible())return {};
    if(nativeBody && nativeBody->window()==owner && nativeBody->isVisible()) {
        const auto content=workspaceViewportBounds(nativeBody);
        return content.isEmpty()?QRect(nativeBody->mapToGlobal(QPoint()),nativeBody->size()):content;
    }
    auto layout=owner->layout();if(!layout)return {};
    QRect content;
    for(int i=0;i<layout->count();++i) {
        auto item=layout->itemAt(i);auto widget=item->widget();
        if(widget) {
            const auto name=widget->objectName();
            if(!widget->isVisibleTo(owner) || name.startsWith("unboundAttachedAccessory_") ||
                name=="openGateNativeFooterMirror")continue;
        } else if(!item->layout())continue;
        content=content.united(item->geometry());
    }
    content=content.intersected(owner->contentsRect());
    return content.isEmpty()?QRect():QRect(owner->mapToGlobal(content.topLeft()),content.size());
}
}
