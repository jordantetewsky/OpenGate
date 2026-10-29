// SPDX-License-Identifier: MIT
#pragma once
#include <QBoxLayout>
#include <QPointer>
#include <QToolButton>
#include "../../core/diagnostics.h"
namespace Unbound {
// The Windows native project strip owns this accessory. Shared WorkspaceHeaders
// owns its menu contents and commands. Never attach to the hideable toolbar or
// place an overlay using guessed caption coordinates.
class WorkspaceTitleAccess : public QObject {
public:
    explicit WorkspaceTitleAccess(QWidget *window):QObject(window),window(window) {
        setObjectName("openGateWorkspaceTitleAccess");
    }
    ~WorkspaceTitleAccess() override { delete button.data(); }
    bool update(bool enabled,std::function<void()> opened) {
        callback=std::move(opened);
        if(!enabled) { delete button.data();button=nullptr;return true; }
        QWidget *strip=nullptr;QBoxLayout *layout=nullptr;
        if(window)for(auto candidate:window->findChildren<QWidget *>("UiProjectsMainBarImp")) {
            auto box=qobject_cast<QBoxLayout *>(candidate->layout());
            if(!candidate->isVisibleTo(window)||!box||
                (box->direction()!=QBoxLayout::LeftToRight&&box->direction()!=QBoxLayout::RightToLeft))continue;
            bool parked=false;
            for(auto parent=candidate;parent&&parent!=window;parent=parent->parentWidget())
                if(parent->objectName()=="unboundNativeLayoutStash"||parent->objectName()=="pageSingleScreenModeTools"||
                   parent->objectName().startsWith("unboundAttachedAccessory_"))parked=true;
            if(parked)continue;
            if(strip) { strip=nullptr;layout=nullptr;break; } // Ambiguous owners fail closed.
            strip=candidate;layout=box;
        }
        if(!strip) {
            delete button.data();button=nullptr;
            if(!unavailableReported)logEvent("header.native-access.unavailable",{{"platform","windows"},{"reason","visible-permanent-project-strip-not-found"}});
            unavailableReported=true;return false;
        }
        if(button&&button->parentWidget()!=strip) { delete button.data();button=nullptr; }
        if(!button) {
            button=new QToolButton(strip);button->setObjectName("openGateTitlePanels");
            button->setText("Panels");button->setAccessibleName("OpenGate Panels");
            button->setAutoRaise(true);button->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(button,0);
            connect(button,&QToolButton::clicked,this,[this] {
                logEvent("header.native-access.clicked",{{"platform","windows"}});
                if(callback)callback();
            });
            logEvent("header.native-access.attached",{{"platform","windows"},{"owner",widgetState(strip)}});
        }
        unavailableReported=false;button->show();return true;
    }
private:
    QPointer<QWidget> window;QPointer<QToolButton> button;
    std::function<void()> callback;bool unavailableReported=false;
};
}
