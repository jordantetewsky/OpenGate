// SPDX-License-Identifier: MIT
#pragma once
#include "nativecommandactivity.h"
#include "timelineclipselection.h"
#include <QEvent>
#include <QTimer>

namespace Unbound {
// Coalesce document/view transitions into assignment reloads. Painting checks
// scene identity only; it never scans or serializes the project each frame.
class TimelineClipColorRestore : public QObject {
  public:
    TimelineClipColorRestore(QGraphicsView *view, QObject *owner, std::function<void()> reload)
        : QObject(owner), root(view), reload(std::move(reload)) {
        if (auto activity = NativeCommandActivity::shared())
            token = activity->subscribe(this, [this](const QString &) { request(); });
        observe();
        request();
    }
    ~TimelineClipColorRestore() override {
        if (auto activity = NativeCommandActivity::existing()) activity->unsubscribe(token);
    }
    void request() {
        if (pending || !root) return;
        pending = true;
        QTimer::singleShot(0, this, [this] {
            pending = false;
            if (!root || !root->isVisible()) return;
            observe();
            if (reload) reload();
        });
    }
  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::Paint) {
            for (auto &entry : observed)
                if (entry.view && (object == entry.view || object == entry.view->viewport()) &&
                    entry.scene != entry.view->scene()) {
                    entry.scene = entry.view->scene();
                    request();
                    break;
                }
        } else if (event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest ||
                   event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved)
            request();
        return false;
    }
  private:
    struct Observed { QPointer<QGraphicsView> view; QPointer<QGraphicsScene> scene; };
    QPointer<QGraphicsView> root;
    QVector<Observed> observed;
    std::function<void()> reload;
    quint64 token = 0;
    bool pending = false;
    void observe() {
        for (int i = observed.size()-1; i >= 0; --i)
            if (!observed[i].view) observed.removeAt(i);
        for (auto view : TimelineClipSelection::views(root)) {
            bool known = false;
            for (const auto &entry : observed) known |= entry.view == view;
            if (!known) {
                observed.append({view, view->scene()});
                view->installEventFilter(this);
                view->viewport()->installEventFilter(this);
            }
        }
    }
};
} // namespace Unbound
