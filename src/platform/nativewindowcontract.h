// SPDX-License-Identifier: MIT
#pragma once
#include <QList>
#include <QJsonObject>
#include <QPoint>
#include <QRect>
#include <functional>
class QString;
class QWidget;
class QObject;
namespace Unbound {
void applyNativeChrome(QWidget *widget, bool compact);
int nativeWorkspaceTopInset(QWidget *page);
bool setNativePin(QWidget *widget, bool pinned, const std::function<void(bool)> &changed);
bool setNativeHeaderMenu(QWidget *widget,bool enabled,const std::function<void()> &opened);
int nativeChromeFlags(QWidget *widget);
void orderNativeWindows(const QList<QWidget *> &bottomToTop);
qint64 nativeKeyWindowNumber();
qint64 nativeWindowNumber(QWidget *widget);
QJsonObject nativeWindowGeometryState(QWidget *widget);
QList<qint64> nativeOrderedWindowNumbers();
double nativeInputAgeMilliseconds(quint64 timestamp);
double nativeCurrentDragAgeMilliseconds();
QObject *watchNativeCommandRequests(QObject *owner,std::function<void()> ready);
QObject *watchNativePointerRelease(QObject *owner, std::function<void(QPoint, bool)> released);
QObject *watchNativeWindowMove(QWidget *window, std::function<void()> begin,
    std::function<void(QPoint)> moved, std::function<void(QPoint, bool)> ended);
bool setNativeGroupSurfaceClipping(QWidget *widget);
void normalizeNativePanel(QWidget *widget);
void setNativeExpansionPriority(QWidget *widget, bool expanded);
int nativeVisibilityFlags(QWidget *widget);
bool setNativeBackgroundHidden(QWidget *main, const QList<QWidget *> &panels, bool hidden);
}
