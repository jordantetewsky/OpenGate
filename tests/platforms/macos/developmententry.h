// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QTimer>
#include <QPointer>
#include <functional>
namespace OpenGateDevelopment {
// Reserve the existing singleton bootstrap name before the installed delayed
// entry runs. Only one runtime constructs the real service; no live replacement.
inline bool reserveEntry(QObject *application,int delay,std::function<void()> start) {
    if(!application || application->findChild<QObject *>("unboundControlService"))return false;
    auto reservation=new QObject(application);reservation->setObjectName("unboundControlService");
    QTimer::singleShot(delay,reservation,[reservation,start=std::move(start)]{
        auto application=reservation->parent();
        reservation->setObjectName("openGateDevelopmentReservation");
        if(!application->findChild<QObject *>("unboundControlService"))start();
        delete reservation;
    });
    return true;
}
}
