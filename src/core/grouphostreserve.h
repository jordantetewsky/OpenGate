// SPDX-License-Identifier: MIT
#pragma once
#include "floatinghost.h"
#include "uiworktiming.h"

namespace Unbound {
// One empty native host, prepared after a quiet interval. Content, layout and
// visibility are never cached here. A cold miss uses the ordinary creation path.
class GroupHostReserve : public QObject
{
public:
    explicit GroupHostReserve(QObject *owner) : QObject(owner)
    {
        timer.setSingleShot(true); timer.setInterval(250);
        connect(&timer,&QTimer::timeout,this,[this] { prepare(); });
        qApp->installEventFilter(this);
    }
    ~GroupHostReserve() override { qApp->removeEventFilter(this); clear(); }
    void request(QWidget *owner)
    {
        if (!owner) return;
        if (nativeOwner != owner) { clear(); nativeOwner = owner; }
        if (!ready && !timer.isActive()) timer.start();
    }
    WorkspaceFloatingHost *take(QWidget *owner)
    {
        WorkspaceFloatingHost *result = nullptr;
        if (nativeOwner == owner && ready) { result = ready; ready = nullptr; }
        request(owner);
        logEvent("group.reserve.take",{{"prepared",bool(result)}});
        return result ? result : new WorkspaceFloatingHost(owner);
    }
    void clear()
    {
        timer.stop(); delete ready.data(); ready = nullptr; nativeOwner = nullptr;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        if (!nativeOwner || ready) return false;
        switch (event->type()) {
        case QEvent::MouseButtonPress: case QEvent::MouseButtonRelease:
        case QEvent::MouseMove: case QEvent::KeyPress:
            timer.start(); break;
        case QEvent::Move: {
            auto widget = qobject_cast<QWidget *>(object);
            if (event->spontaneous() && widget && widget->isWindow() && widget->objectName().startsWith("unboundFloating_")) timer.start();
            break;
        }
        default: break;
        }
        return false;
    }
private:
    void prepare()
    {
        if (!nativeOwner || ready || QCoreApplication::closingDown()) return;
        if (QApplication::mouseButtons() || QWidget::mouseGrabber() ||
            QApplication::activeModalWidget() || QApplication::activePopupWidget()) { timer.start(); return; }
        UiWorkTiming timing("group.reserve.prepare",nullptr);
        ready = new WorkspaceFloatingHost(nativeOwner);
        ready->setObjectName("unboundPreparedGroupHost");
        ready->setAttribute(Qt::WA_QuitOnClose,false);
        ready->setChrome(WindowChrome::Standard);
    }
    QTimer timer;
    QPointer<QWidget> nativeOwner;
    QPointer<WorkspaceFloatingHost> ready;
};
}
