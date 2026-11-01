// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QApplication>
#include <QAction>
#include <QJsonObject>
#include <QLayout>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <optional>
#include <functional>
#include "diagnostics.h"

namespace Unbound {
// Retain intended toolbar/tab presentation across native page transitions. Only
// known descendants of the leased Timeline are reconciled; native choices and
// outer panel/group visibility are never toggled or reset.
class TimelineAttachedPresentation : public QObject {
public:
    TimelineAttachedPresentation() {
        settled.setSingleShot(true); settled.setInterval(250);
        remember.setSingleShot(true); remember.setInterval(250);
        connect(&settled, &QTimer::timeout, this, [this] { reconcile(); });
        connect(&remember, &QTimer::timeout, this, [this] { sample(); });
    }
    ~TimelineAttachedPresentation() override { release(); }
    void attach(QWidget *timeline, QWidget *nativePage) {
        if (!timeline || timeline->objectName() != "frameConformTimelines" || !nativePage || root == timeline)
            return;
        release(); root = timeline; page = nativePage;
        root->installEventFilter(this); page->installEventFilter(this);
        collect(toolbar, "frameEditToolbarTop");
        collect(tabs, "timelineTabsHolder");
        if (toolbar.leaf && !toolbar.wanted) toolbar.wanted = visibleIntent(toolbar);
        if (tabs.leaf && !tabs.wanted) tabs.wanted = visibleIntent(tabs);
        wire();
    }
    void release() {
        settled.stop(); remember.stop();
        if (root) root->removeEventFilter(this);
        if (page) page->removeEventFilter(this);
        for (const auto &connection : connections) disconnect(connection);
        connections.clear(); wired.clear();
        toolbar.path.clear(); tabs.path.clear();
        root = nullptr; page = nullptr;
    }
    QJsonObject snapshot() const {
        QJsonObject result;
        if (toolbar.wanted) result["timeline_toolbar_visible"] = *toolbar.wanted;
        if (tabs.wanted) result["timeline_tabs_visible"] = *tabs.wanted;
        return result;
    }
    void restoreState(const QJsonObject &saved) {
        if (saved.contains("timeline_toolbar_visible")) toolbar.wanted = saved["timeline_toolbar_visible"].toBool();
        if (saved.contains("timeline_tabs_visible")) tabs.wanted = saved["timeline_tabs_visible"].toBool();
        reconcile();
    }
    void pageReturned() {
        if (page && page->isVisible() && !settled.isActive()) { wire(); settled.start(); }
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (object == page) {
            if (event->type() == QEvent::Hide) { settled.stop(); remember.stop(); }
            if (event->type() == QEvent::Show) pageReturned();
        }
        if (object == root && event->type() == QEvent::ChildAdded)
            QTimer::singleShot(0, this, [this] { if (root) wire(); });
        return false;
    }
private:
    struct Node { QPointer<QWidget> widget; int minimum, maximum; };
    struct Band { QPointer<QWidget> leaf; QList<Node> path; std::optional<bool> wanted; };
    void collect(Band &band, const QString &name) {
        band.leaf = root->findChild<QWidget *>(name);
        band.path.clear();
        for (auto widget = band.leaf.data(); widget && widget != root; widget = widget->parentWidget())
            band.path.append({widget, widget->minimumHeight(), widget->maximumHeight()});
    }
    static bool visibleIntent(const Band &band) {
        if (!band.leaf) return false;
        for (const auto &node : band.path) if (node.widget && node.widget->isHidden()) {
            const auto name = node.widget->objectName();
            if (!name.startsWith("unbound") && !name.startsWith("openGate")) return false;
        }
        return true;
    }
    void sample() {
        if (!root || !page || !page->isVisible() || settled.isActive()) return;
        wire();
        const auto tools=visibleIntent(toolbar), strip=visibleIntent(tabs);
        const bool different=toolbar.wanted!=tools || tabs.wanted!=strip;
        toolbar.wanted = tools; tabs.wanted = strip;
        if (different && changed) changed();
    }
    void wire() {
        if(!root)return;
        if(!toolbar.leaf) { collect(toolbar,"frameEditToolbarTop");if(toolbar.leaf&&!toolbar.wanted)toolbar.wanted=visibleIntent(toolbar); }
        if(!tabs.leaf) { collect(tabs,"timelineTabsHolder");if(tabs.leaf&&!tabs.wanted)tabs.wanted=visibleIntent(tabs); }
        auto intent = [this] { if (page && page->isVisible() && !settled.isActive()) remember.start(); };
        for (auto action : root->findChildren<QAction *>()) if (!wired.contains(action)) {
            wired.insert(action);
            connections.append(connect(action, &QAction::triggered, this, intent));
            connections.append(connect(action, &QObject::destroyed, this, [this, action] { wired.remove(action); }));
        }
        for (auto button : root->findChildren<QAbstractButton *>()) if (!wired.contains(button)) {
            wired.insert(button);
            connections.append(connect(button, &QAbstractButton::clicked, this, intent));
            connections.append(connect(button, &QObject::destroyed, this, [this, button] { wired.remove(button); }));
        }
    }
    QWidget *collapseTarget(const Band &band) const {
        for (const auto &node : band.path) if (node.widget) {
            const auto name = node.widget->objectName();
            if (name == "unboundNativePanelPosition_frameEditToolbarTop") return node.widget;
            if (name == "frameHeaderContainer" && (!toolbar.leaf || !node.widget->isAncestorOf(toolbar.leaf))) return node.widget;
        }
        return band.leaf;
    }
    void apply(const Band &band) {
        if (!band.leaf || !band.wanted) return;
        if (!*band.wanted) {
            if (auto target = collapseTarget(band)) target->hide();
            return;
        }
        for (auto it = band.path.crbegin(); it != band.path.crend(); ++it) if (it->widget) {
            if (it->widget->maximumHeight() == 0 && it->maximum > 0) {
                it->widget->setMaximumHeight(it->maximum);
                it->widget->setMinimumHeight(it->minimum);
            }
            it->widget->show();
        }
    }
    void reconcile() {
        if (!root || !page || !page->isVisible()) return;
        wire();
        QPointer<QWidget> focus=QApplication::focusWidget(), active=QApplication::activeWindow();
        apply(toolbar); apply(tabs);
        if (root->layout()) { root->layout()->invalidate(); root->layout()->activate(); }
        if(focus && focus->isVisible() && QApplication::activeWindow()==active)focus->setFocus(Qt::OtherFocusReason);
        logEvent("timeline.attached.presentation", {{"toolbar", toolbar.wanted.value_or(false)},
            {"tabs", tabs.wanted.value_or(false)}, {"native_choice_reset", false}});
    }
public:
    std::function<void()> changed;
private:
    QPointer<QWidget> root, page;
    Band toolbar, tabs;
    QTimer settled, remember;
    QSet<QObject *> wired;
    QList<QMetaObject::Connection> connections;
};
}
