// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QPointer>
#include <QTimer>
#include <functional>
namespace Unbound {
// Short-lived readiness observation; no fixed preparation delay and no retained
// global filter after completion, cancellation or deadline.
class SurfaceReadiness : public QObject {
public:
    SurfaceReadiness(QObject *owner, QWidget *root, std::function<bool()> valid,
        std::function<bool()> ready, std::function<void(bool)> complete, int timeoutMs=1000)
        : QObject(owner), root(root), valid(std::move(valid)), ready(std::move(ready)), complete(std::move(complete)) {
        deadline.setSingleShot(true); deadline.setInterval(timeoutMs);
        connect(&deadline,&QTimer::timeout,this,[this] { finish(false); });
        qApp->installEventFilter(this); deadline.start(); queue();
    }
    ~SurfaceReadiness() override { if (qApp) qApp->removeEventFilter(this); }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        switch (event->type()) {
        case QEvent::Show: case QEvent::Hide: case QEvent::ParentChange:
        case QEvent::LayoutRequest: case QEvent::ChildAdded: break;
        default: return false;
        }
        auto widget=qobject_cast<QWidget *>(object);
        if (root && widget && (widget==root || root->isAncestorOf(widget))) queue();
        return false;
    }
private:
    void queue() {
        if (queued || finished) return;
        queued=true;
        QTimer::singleShot(0,this,[this] {
            queued=false;
            if (!root || !valid()) { finish(false); return; }
            if (ready()) finish(true);
        });
    }
    void finish(bool success) {
        if (finished) return;
        finished=true; deadline.stop(); qApp->removeEventFilter(this);
        auto callback=std::move(complete); deleteLater();
        if (callback) callback(success);
    }
    QPointer<QWidget> root;
    std::function<bool()> valid,ready;
    std::function<void(bool)> complete;
    QTimer deadline;
    bool queued=false,finished=false;
};
}
