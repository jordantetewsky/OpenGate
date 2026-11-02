// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <functional>
namespace Unbound {
// Dispatch once after native menu tracking, bounded by the existing 15 s timeout.
class MenuTrackingCommand {
public:
    static void queue(QObject *owner,std::function<void()> invoke) {
        auto timer=new QTimer(owner);timer->setSingleShot(true);QElapsedTimer elapsed;elapsed.start();
        QObject::connect(timer,&QTimer::timeout,owner,[timer,invoke=std::move(invoke),elapsed]{
            if(QApplication::activePopupWidget() && elapsed.elapsed()<15000){timer->start(25);return;}
            if(!QApplication::activePopupWidget())invoke();
            timer->deleteLater();
        });timer->start(0);
    }
};
}
