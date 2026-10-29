// SPDX-License-Identifier: MIT
#pragma once
#include <QRect>
#include <QList>
#include <QLine>
#include <cstdlib>

namespace Unbound {
struct DockZone { QRect target, preview; Qt::Edge edge = Qt::LeftEdge; bool active = false; };
inline DockZone localDockZone(const QRect &frame, QPoint pointer)
{
    DockZone result;
    if (!frame.isValid() || !frame.adjusted(-12, -12, 12, 12).contains(pointer)) return result;
    // Targets are fixed anchors, not a ribbon following every pointer position.
    const QPoint anchors[] = {QPoint(frame.left(), frame.center().y()), QPoint(frame.right(), frame.center().y()),
        QPoint(frame.center().x(), frame.top()), QPoint(frame.center().x(), frame.bottom()), frame.center()};
    int closest = -1, distance = 27;
    for (int i = 0; i < 5; ++i) {
        const int d = (pointer - anchors[i]).manhattanLength();
        if (d < distance) { distance = d; closest = i; }
    }
    if (closest < 0) return result;
    result.target = QRect(anchors[closest] - QPoint(9, 9), QSize(18, 18));
    result.edge = closest == 0 ? Qt::LeftEdge : closest == 1 ? Qt::RightEdge : closest == 2 ? Qt::TopEdge : closest == 3 ? Qt::BottomEdge : Qt::Edge(0);
    result.preview = frame;
    if (closest < 2) { result.preview.setWidth(frame.width() / 2); if (closest == 1) result.preview.moveRight(frame.right()); }
    else if (closest < 4) { result.preview.setHeight(frame.height() / 2); if (closest == 3) result.preview.moveBottom(frame.bottom()); }
    result.active = result.target.contains(pointer); return result;
}
inline QList<DockZone> dockCompassZones(const QRect &frame)
{
    QList<DockZone> zones;
    const QPoint offsets[] = {QPoint(-30, 0), QPoint(30, 0), QPoint(0, -30), QPoint(0, 30), QPoint()};
    const Qt::Edge edges[] = {Qt::LeftEdge, Qt::RightEdge, Qt::TopEdge, Qt::BottomEdge, Qt::Edge(0)};
    for (int i = 0; i < 5; ++i) {
        DockZone zone; zone.edge = edges[i]; zone.target = QRect(frame.center() + offsets[i] - QPoint(12, 12), QSize(24, 24)); zone.preview = frame;
        if (i < 2) { zone.preview.setWidth(frame.width() / 2); if (i == 1) zone.preview.moveRight(frame.right()); }
        else if (i < 4) { zone.preview.setHeight(frame.height() / 2); if (i == 3) zone.preview.moveBottom(frame.bottom()); }
        zones.append(zone);
    }
    return zones;
}
inline DockZone outerDockZone(const QRect &frame, QPoint pointer)
{
    auto zone = localDockZone(frame.adjusted(-24, -24, 24, 24), pointer);
    if (!zone.target.isValid() || zone.edge == Qt::Edge(0)) return {};
    zone.preview = frame;
    if (zone.edge == Qt::LeftEdge || zone.edge == Qt::RightEdge) {
        zone.preview.setWidth(frame.width() / 2); if (zone.edge == Qt::RightEdge) zone.preview.moveRight(frame.right());
    } else {
        zone.preview.setHeight(frame.height() / 2); if (zone.edge == Qt::BottomEdge) zone.preview.moveBottom(frame.bottom());
    }
    return zone;
}
inline QRect alignWindowEdges(QRect frame, const QList<QRect> &panels, QList<QLine> &guides, int threshold = 6)
{
    int bestX = threshold + 1, bestY = threshold + 1, dx = 0, dy = 0;
    QLine vertical, horizontal;
    for (const auto &target : panels) {
        for (int source : {frame.left(), frame.right()}) for (int edge : {target.left(), target.right()}) {
            if ((source == frame.left()) != (edge == target.left()) && frame.bottom() >= target.top() && frame.top() <= target.bottom()) continue;
            const int distance = std::abs(edge - source);
            if (distance < bestX) { bestX = distance; dx = edge - source;
                vertical = QLine(edge, qMin(frame.top(), target.top()), edge, qMax(frame.bottom(), target.bottom())); }
        }
        for (int source : {frame.top(), frame.bottom()}) for (int edge : {target.top(), target.bottom()}) {
            if ((source == frame.top()) != (edge == target.top()) && frame.right() >= target.left() && frame.left() <= target.right()) continue;
            const int distance = std::abs(edge - source);
            if (distance < bestY) { bestY = distance; dy = edge - source;
                horizontal = QLine(qMin(frame.left(), target.left()), edge, qMax(frame.right(), target.right()), edge); }
        }
    }
    if (bestX <= threshold) guides.append(vertical);
    if (bestY <= threshold) guides.append(horizontal);
    return frame.translated(dx, dy);
}
inline QRect snapWindowFrame(QRect frame, const QList<QRect> &panels, const QList<QRect> &displays, int threshold = 12)
{
    int dx = 0, dy = 0, bestX = threshold + 1, bestY = threshold + 1;
    const auto x = [&](int offset) { if (std::abs(offset) < bestX) { bestX = std::abs(offset); dx = offset; } };
    const auto y = [&](int offset) { if (std::abs(offset) < bestY) { bestY = std::abs(offset); dy = offset; } };
    for (const auto &target : panels) {
        if (frame.bottom() >= target.top() - threshold && frame.top() <= target.bottom() + threshold) {
            x(target.right() + 1 - frame.left()); x(target.left() - frame.right() - 1);
            x(target.left() - frame.left()); x(target.right() - frame.right());
        }
        if (frame.right() >= target.left() - threshold && frame.left() <= target.right() + threshold) {
            y(target.bottom() + 1 - frame.top()); y(target.top() - frame.bottom() - 1);
            y(target.top() - frame.top()); y(target.bottom() - frame.bottom());
        }
    }
    for (const auto &display : displays) if (display.adjusted(-threshold, -threshold, threshold, threshold).intersects(frame)) {
        x(display.left() - frame.left()); x(display.right() - frame.right());
        y(display.top() - frame.top()); y(display.bottom() - frame.bottom());
    }
    return frame.translated(dx, dy);
}
}
