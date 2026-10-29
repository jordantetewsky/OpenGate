// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>
#include "../../platform/nativewindowcontract.h"
#include "panefloatingwindow.h"
#include "panetaskbar.h"
namespace Unbound {
class WorkspaceHostWindow : public PaneFloatingWindow {
public:
    WorkspaceHostWindow(QWidget *owner,Qt::WindowFlags flags) {
        flags=(flags & ~Qt::WindowType_Mask)|Qt::Window|Qt::CustomizeWindowHint|Qt::WindowSystemMenuHint;
        setParent(owner,flags);setNativeTitleBarEnabled(true);
        if(owner)inheritPaneTaskbarIdentity(this,owner);
    }
};
// Qt excludes the Windows caption already. Resolve's own project strip can
// still overlap a page inside the client rectangle; reserve only that measured
// overlap, independently of native caption height or DPI.
inline int nativeWorkspaceTopInset(QWidget *page) {
    if(!page)return 0;
    auto main=page->window();int bottom=0;
    for(auto strip:main->findChildren<QWidget *>("UiProjectsMainBarImp",Qt::FindDirectChildrenOnly))
        if(strip->isVisible())bottom=qMax(bottom,strip->mapTo(main,QPoint(0,strip->height())).y());
    return qMax(0,bottom-page->mapTo(main,QPoint()).y());
}
}
