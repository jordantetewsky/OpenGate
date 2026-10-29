// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
#include "droppolicy.h"
namespace Unbound {
inline QString dropCaption(DropPlacement placement, const QString &title, bool wholeGroup)
{
    const auto destination = wholeGroup ? QStringLiteral("entire group") : title;
    if (placement == DropPlacement::Tab) return QStringLiteral("Add as tab to %1").arg(destination);
    const auto direction = placement == DropPlacement::Left ? QStringLiteral("left of") :
        placement == DropPlacement::Right ? QStringLiteral("right of") :
        placement == DropPlacement::Above ? QStringLiteral("above") :
        placement == DropPlacement::Below ? QStringLiteral("below") : QString();
    return direction.isEmpty() ? QString() : QStringLiteral("Dock %1 %2").arg(direction, destination);
}
}
