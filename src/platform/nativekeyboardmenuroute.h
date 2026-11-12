// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <functional>
namespace Unbound {
// Adapts OS-owned application-menu actions that bypass Qt menu tracking.
class NativeKeyboardMenuRoute : public QObject {
public:
    using QObject::QObject;
    virtual void abandonForQuit()=0;
};
#ifdef Q_OS_MAC
NativeKeyboardMenuRoute *createNativeKeyboardMenuRoute(QObject *owner,std::function<bool()> open);
#else
inline NativeKeyboardMenuRoute *createNativeKeyboardMenuRoute(QObject *,std::function<bool()>) {return nullptr;}
#endif
}
