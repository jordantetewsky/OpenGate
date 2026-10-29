// SPDX-License-Identifier: MIT
#pragma once
#include <QRect>
#include <array>

namespace Unbound {
enum class DropPlacement { None, Left, Right, Above, Below, Tab };
struct DropSelection {
    DropPlacement placement = DropPlacement::None;
    QRect region, preview;
};

// Geometry only. The caller resolves a visible destination before invoking this.
inline DropSelection selectPanelDrop(const QRect &surface, QPoint pointer,
    DropPlacement previous = DropPlacement::None)
{
    if (surface.width() < 8 || surface.height() < 8 || !surface.contains(pointer)) return {};
    const int x = qMin(40, qMax(1, surface.width() / 5));
    const int y = qMin(40, qMax(1, surface.height() / 5));
    const QRect center = surface.adjusted(x, y, -x, -y);
    QRect tab(QPoint(),QSize(qMin(160,center.width()),qMin(96,center.height())));tab.moveCenter(center.center());
    const std::array<DropSelection, 5> regions{{
        {DropPlacement::Left, QRect(surface.left(), surface.top(), x, surface.height()),
            QRect(surface.left(), surface.top(), surface.width()/2, surface.height())},
        {DropPlacement::Right, QRect(surface.right()-x+1, surface.top(), x, surface.height()),
            QRect(surface.left()+surface.width()/2, surface.top(), surface.width()-surface.width()/2, surface.height())},
        {DropPlacement::Above, QRect(center.left(), surface.top(), center.width(), y),
            QRect(surface.left(), surface.top(), surface.width(), surface.height()/2)},
        {DropPlacement::Below, QRect(center.left(), surface.bottom()-y+1, center.width(), y),
            QRect(surface.left(), surface.top()+surface.height()/2, surface.width(), surface.height()-surface.height()/2)},
        {DropPlacement::Tab, tab, surface}
    }};
    // Retention is clipped to the destination: leaving it always means free floating.
    for (const auto &region : regions)
        if (region.placement == previous && region.region.adjusted(-4,-4,4,4).contains(pointer)) return region;
    for (const auto &region : regions) if (region.region.contains(pointer)) return region;
    return {};
}
}
