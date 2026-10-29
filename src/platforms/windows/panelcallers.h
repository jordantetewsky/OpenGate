// SPDX-License-Identifier: MIT
#pragma once
#include <QMenuBar>
#include <QSet>
#include "../../core/workspacepanelcallers.h"
#include "../../core/nativepaneltoggles.h"
#include "../../core/sidebarfloatsession.h"
#include "../../core/pagecontext.h"
namespace Unbound {
// Windows discovery supplies real endpoints; shared callers own command and
// native-header intent. This adapter does not own a layout or panel content.
class WindowsPanelCallers : public QObject {
public:
    struct Endpoint {
        QString id,label,page;
        std::function<QWidget *()> pane;
        std::function<SidebarFloatSession *()> session;
        std::function<bool(QAbstractButton *)> matches;
        std::function<void(bool)> setNativeVisible;
        std::function<void(bool)> saveHidden;
    };
    explicit WindowsPanelCallers(QObject *owner) : QObject(owner), callers(this), native(this) {
        callers.currentPage=[] { return currentWorkspacePage(resolveMainWindow()); };
        callers.report=[](QString event,QJsonObject details){logEvent(event,details);};
    }
    void add(Endpoint endpoint) {
        auto active=[endpoint] { auto s=endpoint.session();return s&&s->active(); };
        auto shown=[endpoint] {
            auto s=endpoint.session();if(s&&s->active())return !s->hiddenByUser();
            auto p=endpoint.pane();return p&&p->isVisible();
        };
        auto apply=[this,endpoint](bool visible) {
            auto s=endpoint.session();
            if(s&&s->active()) {
                if(s->setHiddenByUser(!visible)) {
                    endpoint.saveHidden(!visible);
                    logEvent(visible?"panel.show":"panel.hide",{{"panel",endpoint.id},
                        {"stage","session-state"},{"requested_hidden",!visible},
                        {"host",widgetState(s->floatingWindow(),false)}});
                }
            } else endpoint.setNativeVisible(visible);
            if(main)native.sync(main);
        };
        callers.add({endpoint.id,endpoint.label,endpoint.page,[endpoint]{return endpoint.pane()!=nullptr;},shown,
            [apply]{apply(true);},apply});
        native.addMatching(endpoint.id,endpoint.matches,active,shown,[shown,apply]{apply(!shown());});
    }
    void refresh(QWidget *window) {
        // Development gate until managed workspace startup/ownership is wired.
        // Keep the established floating-backend fixture contract independently
        // verifiable while the new assembled caller path is exercised explicitly.
        if(qEnvironmentVariableIntValue("OPENGATE_WINDOWS_MANAGED_CALLERS")!=1)return;
        if(!window)return;main=window;native.wire(window);
        // Use the existing native Qt menu bar. Permanent title-strip access is
        // a separate primary-workspace integration gate, not claimed here.
        auto bars=window->findChildren<QMenuBar *>();
        if(bars.size()!=1)return;
        auto bar=bars.first();if(menus.contains(bar))return;menus.insert(bar);
        auto menu=bar->addMenu("Panels");menu->setObjectName("openGatePanels");menu->setProperty("unboundOwnMenu",true);
        connect(menu,&QMenu::aboutToShow,this,[this,menu]{menu->clear();callers.populateFlat(menu);});
        connect(bar,&QObject::destroyed,this,[this,bar]{menus.remove(bar);});
    }
    void sync() {if(main)native.sync(main);}
private:
    WorkspacePanelCallers callers;
    NativePanelToggles native;
    QPointer<QWidget> main;
    QSet<QMenuBar *> menus;
};
using PlatformPanelCallers = WindowsPanelCallers;
}
