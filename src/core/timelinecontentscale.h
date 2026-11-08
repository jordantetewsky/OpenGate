// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QTimer>
#include <memory>
#include <vector>
#include "nativepanelposition.h"
#include "panelancestry.h"
#include "timelinecontentviewport.h"

namespace Unbound {
// Reversible view leases and one retained header configuration across stacked
// Timelines. Focused adapters own drawing, geometry and input.
class TimelineContentScale : public QObject {
public:
    TimelineContentScale() {
        pending.setSingleShot(true);
        connect(&pending, &QTimer::timeout, this, [this] {
            if (root && !applying) apply(root, amount);
        });
    }
    ~TimelineContentScale() override { restore(); }
    void abandonForQuit() {
        pending.stop();
        for(auto widget:watched)if(widget)widget->removeEventFilter(this);
        watched.clear(); changed={}; root=nullptr;
        for(auto &entry:entries) {
            if(entry->viewport)entry->viewport->changed={};
            entry->position.abandonForQuit();
        }
        entries.clear();
    }
    std::function<void()> changed;
    static QList<QGraphicsView *> surfaces(QWidget *root) {
        QList<QGraphicsView *> result;
        if (root) for (auto view : root->findChildren<QGraphicsView *>("m_TiTimelineView"))
            if (view->scene() && view->viewport()) result.append(view);
        return result;
    }
    static bool supported(QWidget *root) { return !surfaces(root).isEmpty(); }
    QJsonObject snapshot() const {
        if (headerWidth == 0. && headerOffset == 0. && pinned && headerScale==0.) return {};
        return {{"timeline_header_width", headerWidth}, {"timeline_header_scroll", headerOffset},
                {"timeline_header_pinned", pinned},{"timeline_header_scale",headerScale}};
    }
    void restoreState(const QJsonObject &saved) {
        headerWidth = qBound(0., saved["timeline_header_width"].toDouble(), 4096.);
        headerOffset = qBound(0., saved["timeline_header_scroll"].toDouble(), 4096.);
        pinned = saved["timeline_header_pinned"].toBool(true);
        const auto header=saved["timeline_header_scale"].toDouble();
        headerScale=qIsFinite(header) && header>=.7 && header<=3.?header:0.;
        configure();
    }
    bool apply(QWidget *surface, double factor) {
        if (!surface || !qIsFinite(factor) || factor < .7 || factor > 3.) return false;
        if (root != surface) {
            if (!restore()) return false;
            root = surface;
        }
        amount = factor;
        watch(surface);
        const auto candidates = surfaces(surface);
        // A native Timeline scene can arrive after workspace restoration. Keep
        // its requested state and attach at the first ready viewport event.
        if (candidates.isEmpty()) return true;
        QScopedValueRollback<bool> guard(applying, true);
        for (auto view : candidates) {
            bool known = false;
            for (const auto &entry : entries)
                if (entry->position.content() == view) known = true;
            if (known) continue;
            QPointer<QWidget> focus = QApplication::focusWidget();
            if (focus && !panelContains(view, focus)) focus = nullptr;
            auto entry = std::make_unique<Entry>();
            const auto error = entry->position.acquire(view, surface, false, true);
            if (!error.isEmpty()) {
                logEvent("timeline.content.viewport.unavailable", {{"reason", error}});
                continue;
            }
            auto slot = entry->position.surface();
            auto layout = new QVBoxLayout(slot);
            layout->setContentsMargins(0, 0, 0, 0);
            entry->viewport = new TimelineContentViewport(view, slot);
            layout->addWidget(entry->viewport);
            auto presentation = entry->viewport;
            presentation->changed = [this, presentation] {
                if (!presentation || applying) return;
                headerWidth = presentation->retainedHeaderWidth();
                headerOffset = presentation->headerOffset();
                pinned = presentation->headerPinned();
                headerScale=presentation->headerScalePreference();
                configure();
                if (changed) changed();
            };
            presentation->show();
            slot->show();
            if (focus) focus->setFocus(Qt::OtherFocusReason);
            entries.push_back(std::move(entry));
        }
        configure();
        return !entries.empty();
    }
    bool restore() {
        pending.stop();
        for (auto widget : watched) if (widget) widget->removeEventFilter(this);
        watched.clear();
        bool complete = true;
        for (auto it = entries.begin(); it != entries.end();) {
            auto &entry = **it;
            auto view = entry.position.content();
            if (!entry.position.active() && !view) {
                it = entries.erase(it);
                continue;
            }
            if (!view || (entry.viewport && view->parentWidget() != entry.viewport) ||
                (!entry.viewport && view->parentWidget() != entry.position.contentParking())) {
                complete = false;
                ++it;
                continue;
            }
            QPointer<QWidget> focus = QApplication::focusWidget();
            if (focus && !panelContains(view, focus)) focus = nullptr;
            if (entry.viewport) {
                view->setParent(entry.position.contentParking());
                delete entry.viewport.data();
                delete entry.position.surface()->layout();
            }
            if (!entry.position.restore()) {
                complete = false;
                ++it;
                continue;
            }
            if (focus) focus->setFocus(Qt::OtherFocusReason);
            it = entries.erase(it);
        }
        if (complete) root = nullptr;
        return complete;
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!applying && root && (event->type() == QEvent::ChildAdded ||
            event->type() == QEvent::ChildPolished || event->type() == QEvent::Show)) pending.start(0);
        if (!applying && root && event->type() == QEvent::Paint) {
            auto widget=qobject_cast<QWidget *>(object);
            auto view=widget?qobject_cast<QGraphicsView *>(widget->parentWidget()):nullptr;
            if(view && view->objectName()=="m_TiTimelineView" && view->scene()) {
                bool known=false;
                for(const auto &entry:entries)if(entry->position.content()==view)known=true;
                if(!known)pending.start(0);
            }
        }
        return false;
    }
private:
    struct Entry {
        NativePanelPosition position;
        QPointer<TimelineContentViewport> viewport;
    };
    void configure() {
        for (const auto &entry : entries)
            if (entry->viewport) entry->viewport->configure(amount, headerWidth, headerOffset, pinned,headerScale);
    }
    void watch(QWidget *surface) {
        auto widgets = surface->findChildren<QWidget *>();
        widgets.prepend(surface);
        for (auto widget : widgets) {
            const auto name = widget->objectName();
            const bool nativeViewport=widget->parentWidget() && widget->parentWidget()->objectName()=="m_TiTimelineView";
            if (widget != surface && name != "UiTiLayoutContainer" && name != "timelineViewsSplitter" &&
                name != "frameEditTimelinesContainer" && name != "m_TiTimelineView" && !nativeViewport) continue;
            if (!watched.contains(widget)) {
                watched.append(widget);
                widget->installEventFilter(this);
            }
        }
    }
    QPointer<QWidget> root;
    QTimer pending;
    QList<QPointer<QWidget>> watched;
    std::vector<std::unique_ptr<Entry>> entries;
    double amount = 1., headerWidth = 0., headerOffset = 0.,headerScale=0.;
    bool pinned = true, applying = false;
};
}
