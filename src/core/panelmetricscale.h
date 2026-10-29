// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QChildEvent>
#include <QComboBox>
#include <QLayout>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Scale native widget metrics, never a rendered screenshot or the application.
// Each value is derived from a retained baseline, so repeated +/- never drifts.
// Custom-painted pixel constants remain owned by Resolve; no GPU/input transform.
class PanelMetricScale : public QObject {
public:
    explicit PanelMetricScale(QObject *owner=nullptr) : QObject(owner) {
        discover.setSingleShot(true);
        connect(&discover,&QTimer::timeout,this,[this] { if (root) scan(root); });
    }
    ~PanelMetricScale() override { restore(); }
    double factor() const { return amount; }
    bool attach(QWidget *surface,double scale) {
        if (!surface || !qIsFinite(scale) || scale<0.7 || scale>3.0) return false;
        if (root!=surface) { restore(); root=surface; }
        amount=scale; scan(surface); apply();
        logEvent("panel.scale.applied",{{"panel",surface->objectName()},{"factor",amount},
            {"widgets",widgets.size()},{"layouts",layouts.size()},
            {"method","native-font-control-layout-metrics"},{"custom_paint_coverage","live-verification-required"}});
        return true;
    }
    void restore() {
        discover.stop();
        for (const auto &entry:widgets) if (auto w=entry.widget.data()) {
            w->removeEventFilter(this);
            w->setMinimumSize(QSize(0,0)); w->setMaximumSize(entry.maximum); w->setMinimumSize(entry.minimum);
            w->setFont(entry.font); w->setAttribute(Qt::WA_SetFont,entry.explicitFont);
            if (auto button=qobject_cast<QAbstractButton *>(w)) button->setIconSize(entry.icon);
            if (auto combo=qobject_cast<QComboBox *>(w)) combo->setIconSize(entry.icon);
            if (auto view=qobject_cast<QAbstractItemView *>(w)) view->setIconSize(entry.icon);
            w->updateGeometry();
        }
        for (const auto &entry:layouts) if (entry.layout) {
            entry.layout->setContentsMargins(entry.margins); entry.layout->setSpacing(entry.spacing); entry.layout->invalidate();
        }
        if (root) logEvent("panel.scale.restored",{{"panel",root->objectName()},{"widgets",widgets.size()}});
        widgets.clear(); layouts.clear(); known.clear(); knownLayouts.clear(); root=nullptr; amount=1.0;
    }
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        // ChildAdded arrives before the derived QWidget constructor completes.
        if (event->type()==QEvent::ChildAdded || event->type()==QEvent::ChildPolished) discover.start(0);
        return false;
    }
private:
    struct Widget { QPointer<QWidget> widget; QFont font; QSize minimum,maximum,icon; bool explicitFont; };
    struct Layout { QPointer<QLayout> layout; QMargins margins; int spacing; };
    QPointer<QWidget> root; QList<Widget> widgets; QList<Layout> layouts;
    QSet<QObject *> known,knownLayouts; QTimer discover; double amount=1.0;
    int scaled(int value) const { return value<=0 || value>=QWIDGETSIZE_MAX ? value : qMax(1,qRound(value*amount)); }
    QSize size(QSize value) const { return {scaled(value.width()),scaled(value.height())}; }
    void scanLayout(QLayout *layout) {
        if (!layout) return;
        if (!knownLayouts.contains(layout)) {
            knownLayouts.insert(layout); layouts.append({layout,layout->contentsMargins(),layout->spacing()});
            connect(layout,&QObject::destroyed,this,[this,layout] { knownLayouts.remove(layout); });
        }
        for (int i=0;i<layout->count();++i) if (auto child=layout->itemAt(i)->layout()) scanLayout(child);
    }
    void scan(QWidget *surface) {
        QList<QWidget *> all{surface}; all.append(surface->findChildren<QWidget *>());
        for (auto widget:all) {
            bool excluded=false;
            for (auto ancestor=widget;ancestor && ancestor!=surface;ancestor=ancestor->parentWidget())
                if (ancestor->isWindow() || ancestor->property("openGateChrome").toBool()) { excluded=true; break; }
            if (excluded || widget->property("openGateChrome").toBool()) continue;
            // A viewport adapter derives its own geometry from the scaled native
            // child. Scale the child, not the adapter's already-derived height.
            if(widget->property("openGateMetricContainer").toBool())continue;
            if (!known.contains(widget)) {
                QSize icon;
                if (auto button=qobject_cast<QAbstractButton *>(widget)) icon=button->iconSize();
                if (auto combo=qobject_cast<QComboBox *>(widget)) icon=combo->iconSize();
                if (auto view=qobject_cast<QAbstractItemView *>(widget)) icon=view->iconSize();
                auto font=widget->font();
                if (!widget->testAttribute(Qt::WA_SetFont)) for (auto ancestor=widget->parentWidget();ancestor;ancestor=ancestor->parentWidget()) {
                    bool found=false;
                    for (const auto &entry:widgets) if (entry.widget==ancestor) { font=entry.font; found=true; break; }
                    if (found || ancestor==surface) break;
                }
                known.insert(widget); widgets.append({widget,font,widget->minimumSize(),widget->maximumSize(),icon,widget->testAttribute(Qt::WA_SetFont)});
                widget->installEventFilter(this);
                connect(widget,&QObject::destroyed,this,[this,widget] { known.remove(widget); });
            }
            scanLayout(widget->layout());
        }
        apply();
    }
    void apply() {
        // Descendants have their own retained resolved font: parent propagation
        // must not become the next child's baseline during this transaction.
        for (const auto &entry:widgets) if (auto w=entry.widget.data()) {
            auto font=entry.font;
            if (font.pixelSize()>0) font.setPixelSize(scaled(font.pixelSize()));
            else if (font.pointSizeF()>0) font.setPointSizeF(entry.font.pointSizeF()*amount);
            w->setFont(font);
            w->setMinimumSize(QSize(0,0)); w->setMaximumSize(size(entry.maximum)); w->setMinimumSize(size(entry.minimum));
            if (entry.icon.isValid()) {
                if (auto button=qobject_cast<QAbstractButton *>(w)) button->setIconSize(size(entry.icon));
                if (auto combo=qobject_cast<QComboBox *>(w)) combo->setIconSize(size(entry.icon));
                if (auto view=qobject_cast<QAbstractItemView *>(w)) view->setIconSize(size(entry.icon));
            }
            w->updateGeometry();
        }
        for (const auto &entry:layouts) if (entry.layout) {
            const auto m=entry.margins; entry.layout->setContentsMargins(scaled(m.left()),scaled(m.top()),scaled(m.right()),scaled(m.bottom()));
            entry.layout->setSpacing(scaled(entry.spacing)); entry.layout->invalidate();
        }
    }
};
}
