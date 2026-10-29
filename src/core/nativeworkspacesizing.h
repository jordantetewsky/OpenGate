// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QLayout>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <functional>
#include "diagnostics.h"
#include "cinemaviewer.h"
#include "nativevacancycollapse.h"
namespace Unbound {
// Scoped to a native page with managed panels. Never changes content constraints
// or floating group sizes, and never raises/activates the native window.
class NativeWorkspaceSizing : public QObject {
public:
    explicit NativeWorkspaceSizing(QObject *owner) : QObject(owner) {
        settle.setSingleShot(true);
        connect(&settle,&QTimer::timeout,this,[this] { apply(); });
    }
    ~NativeWorkspaceSizing() override { restore(); }
    std::function<bool()> blocked = [] { return false; };
    void prepare(QWidget *window,QWidget *nativePage) {
        if (window!=main || nativePage!=page) {
            restore(); main=window; page=nativePage;
            if (main) originalGeometry=main->geometry();
            if (page) page->installEventFilter(this);
        }
        if (!captured && main && main->layout()) {
            originalMinimum=main->minimumSize(); originalLayout=main->layout();
            originalConstraint=originalLayout->sizeConstraint(); captured=true;
        }
    }
    void setPanels(QWidget *window,QWidget *nativePage,const QSet<QString> &ids,const QList<QWidget *> &vacancies={}) {
        if (ids.isEmpty()) { restore(); return; }
        const bool unchanged=window==main && nativePage==page && ids==managedPanels;
        prepare(window,nativePage);
        collapse.update(nativePage,vacancies);
        if (unchanged && applied) return;
        managedPanels=ids; settle.start(0);
    }
    void restore() {
        settle.stop(); collapse.restore(); restoreConstraints(true);
        if (page) page->removeEventFilter(this);
        page=nullptr; main=nullptr; managedPanels.clear();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (object!=page) return false;
        if (event->type()==QEvent::Hide) { settle.stop(); collapse.restore(); restoreConstraints(false); }
        if (event->type()==QEvent::Show && !managedPanels.isEmpty()) settle.start(0);
        return false;
    }
private:
    void apply() {
        if ((blocked && blocked()) || !main || !page || !page->isVisible() || managedPanels.isEmpty() || visibleCinemaViewer() ||
            QApplication::activeModalWidget() || QApplication::activePopupWidget()) return;
        auto layout=main->layout(); if (!layout) return;
        if (!captured) {
            originalMinimum=main->minimumSize();
            originalConstraint=layout->sizeConstraint(); originalLayout=layout; captured=true;
        }
        if (originalLayout!=layout) {
            logEvent("workspace.native-sizing.cancelled",{{"reason","native-layout-replaced"}});
            restoreConstraints(false); return;
        }
        const auto before=main->geometry();
        layout->setSizeConstraint(QLayout::SetNoConstraint); applied=true;
        // The real surviving native layout determines the floor. No child control
        // minima are lowered just to make the outer window appear smaller.
        const auto required=layout->minimumSize().expandedTo(QSize(320,200));
        main->setMinimumSize(required); layout->activate();
        // Reclaim space inside the user's frame. Lower the resize floor, but
        // never shrink the main workspace just because a panel was detached.

        logEvent("workspace.native-sizing.applied",{{"managed_panel_count",managedPanels.size()},
            {"minimum_width",required.width()},{"minimum_height",required.height()},
            {"width_before",before.width()},{"height_before",before.height()},
            {"width_after",main->width()},{"height_after",main->height()},
            {"scope","native outer window; surviving content constraints preserved"}});
    }
    void restoreConstraints(bool geometry) {
        if (!captured) return;
        if (main) {
            const bool unchanged=automaticGeometry && main->geometry()==lastAutomaticGeometry;
            if (originalLayout && main->layout()==originalLayout) originalLayout->setSizeConstraint(originalConstraint);
            main->setMinimumSize(originalMinimum);
            const bool restoredGeometry=geometry && unchanged && !main->isMaximized() && !main->isFullScreen();
            if (restoredGeometry) main->setGeometry(originalGeometry);
            logEvent("workspace.native-sizing.restored",{{"minimum_width",main->minimumWidth()},
                {"minimum_height",main->minimumHeight()},{"geometry_restored",restoredGeometry}});
        }
        captured=false; applied=false; automaticGeometry=false; originalLayout=nullptr;
    }
    NativeVacancyCollapse collapse;
    QPointer<QWidget> main,page; QPointer<QLayout> originalLayout;
    QSet<QString> managedPanels; QTimer settle; QSize originalMinimum;
    QRect originalGeometry,lastAutomaticGeometry;
    QLayout::SizeConstraint originalConstraint=QLayout::SetDefaultConstraint;
    bool captured=false,applied=false,automaticGeometry=false;
};
}
