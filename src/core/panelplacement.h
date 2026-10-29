// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
#include <QJsonObject>
namespace Unbound {
enum class PanelPlacement { NativeSlot, NativeSurface, FloatingSurface };
inline QString panelPlacementName(PanelPlacement placement) {
    switch (placement) {
    case PanelPlacement::NativeSlot: return "native-slot";
    case PanelPlacement::NativeSurface: return "native-surface";
    case PanelPlacement::FloatingSurface: return "floating-surface";
    }
    return {};
}
// Visibility and temporary expansion do not change durable placement. The
// original native slot is an ownership/restoration contract, not a surface ID.
struct PanelPresentation {
    PanelPlacement placement=PanelPlacement::NativeSlot;
    QString surface;
    bool visible=true;
    bool expanded=false;
    QJsonObject snapshot() const {
        return {{"placement",panelPlacementName(placement)},{"surface",surface},{"visible",visible},{"expanded",expanded}};
    }
    bool move(PanelPlacement destination,const QString &surfaceId={}) {
        if (destination!=PanelPlacement::NativeSlot && surfaceId.isEmpty()) return false;
        if (destination==PanelPlacement::NativeSlot && !surfaceId.isEmpty()) return false;
        placement=destination; surface=surfaceId; return true;
    }
};
}
