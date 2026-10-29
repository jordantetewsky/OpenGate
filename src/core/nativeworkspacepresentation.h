// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include "../platform/nativewindowcontract.h"
#include "nativeworkspacesizing.h"
#include "panelsession.h"
#include "diagnostics.h"
namespace Unbound {
// The native main window is the workspace while it contains editing surfaces.
// Once every visible editing surface has left, retire only its empty shell.
// Manual hide-background remains a separate user override/recovery facility.
class NativeWorkspacePresentation {
public:
    ~NativeWorkspacePresentation() { restore(); }
    std::function<bool(QWidget *,const QList<QWidget *> &,bool)> visibility=setNativeBackgroundHidden;
    void update(QWidget *main,bool enabled,bool editPage,bool nativeArrangement,bool retainedContent,
                int leasedPanels,const QList<QWidget *> &floating,bool manualHide) {
        if (window && window!=main) restore();
        window=main;
        const bool empty=enabled && editPage && !nativeArrangement && !retainedContent && leasedPanels>0 && !floating.isEmpty();
        if (manualHide) { automaticallyHidden=false; return; }
        if (empty==automaticallyHidden || !main) return;
        const bool success=visibility(main,floating,empty);
        if (success) automaticallyHidden=empty;
        logEvent("workspace.native-shell.presentation",{{"empty",empty},{"success",success},
            {"leased_panels",leasedPanels},{"retained_content",retainedContent},{"automatic",true}});
    }
    struct Surface { QString id; QWidget *native=nullptr; ManagedPanelSession *session=nullptr; bool toolbar=false; };
    void reconcile(QWidget *main,QWidget *page,NativeWorkspaceSizing *sizing,bool nativeArrangement,
                   const QList<Surface> &surfaces,bool manualHide,bool keepPrimary=false) {
        QList<QWidget *> floating,vacancies; QSet<QString> managed; bool timelineKnown=false;
        for (const auto &surface:surfaces) {
            auto session=surface.session; if (!session) continue;
            if (surface.id=="frameConformTimelines") timelineKnown=session->active() || surface.native;
            if (!session->active()) continue;
            managed.insert(surface.id); vacancies.append(session->nativeVacancy());
            auto window=session->window();
            if (window && window->isVisible() && window->window()!=main && !floating.contains(window->window())) floating.append(window->window());
        }
        if (sizing && page && !nativeArrangement) sizing->setPanels(main,page,managed,vacancies);
        // Collapse/reopen native ancestors before determining whether any real
        // native panel remains. A native toggle must restore its shell in the
        // same transaction, not depend on a second mouse event.
        bool retained=false;
        for (const auto &surface:surfaces) if (surface.session && !surface.session->active() && !surface.toolbar)
            retained |= surface.native && surface.native->isVisible();
        update(main,true,page && page->isVisible() && timelineKnown,nativeArrangement || keepPrimary,retained,managed.size(),floating,manualHide);
    }
    void restore() {
        if (window && automaticallyHidden) visibility(window,{},false);
        automaticallyHidden=false; window=nullptr;
    }
    bool hiddenAutomatically() const { return automaticallyHidden; }
private:
    QPointer<QWidget> window; bool automaticallyHidden=false;
};
}
