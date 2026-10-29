// SPDX-License-Identifier: MIT
#pragma once
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <limits>

namespace Unbound {
inline QRect fitToDisplays(QRect desired, const QList<QRect> &displays)
{
    if (displays.isEmpty()) return desired;
    QRect best = displays.first();
    qint64 bestArea = -1, bestDistance = std::numeric_limits<qint64>::max();
    for (const QRect &display : displays) {
        const QRect intersection = desired.intersected(display);
        const qint64 area = qint64(intersection.width()) * intersection.height();
        const QPoint delta = desired.center() - display.center();
        const qint64 distance = qint64(delta.x()) * delta.x() + qint64(delta.y()) * delta.y();
        if (area > bestArea || (area == bestArea && distance < bestDistance)) {
            best = display; bestArea = area; bestDistance = distance;
        }
    }
    desired.setSize(desired.size().boundedTo(best.size()).expandedTo(QSize(1, 1)));
    desired.moveLeft(qBound(best.left(), desired.left(), best.right() - desired.width() + 1));
    desired.moveTop(qBound(best.top(), desired.top(), best.bottom() - desired.height() + 1));
    return desired;
}

// Keep a useful title/grip segment reachable; partial off-screen placement is intentional.
inline QRect keepGripReachable(QRect desired, const QList<QRect> &displays)
{
    if (displays.isEmpty() || !desired.isValid()) return desired;
    const int gripWidth = qMin(96, desired.width()), gripHeight = qMin(24, desired.height());
    QRect best = desired; qint64 distance = std::numeric_limits<qint64>::max();
    for (const auto &display : displays) {
        QRect candidate = desired;
        candidate.moveLeft(qBound(display.left() + gripWidth - desired.width(), desired.left(), display.right() - gripWidth + 1));
        candidate.moveTop(qBound(display.top(), desired.top(), display.bottom() - gripHeight + 1));
        const QPoint delta = candidate.topLeft() - desired.topLeft();
        const qint64 d = qint64(delta.x()) * delta.x() + qint64(delta.y()) * delta.y();
        if (d < distance) { best = candidate; distance = d; }
    }
    return best;
}

inline void keepOnDisplay(QWidget *window)
{
    if (!window) return;
    QList<QRect> areas;
    for (auto screen : QGuiApplication::screens()) areas.append(screen->availableGeometry());
    const QRect frame = window->frameGeometry();
    const QRect fitted = keepGripReachable(frame, areas);
    if (frame == fitted) return;
    const QSize decoration = frame.size() - window->size();
    window->resize((fitted.size() - decoration).expandedTo(QSize(1, 1)));
    window->move(fitted.topLeft());
}
}
