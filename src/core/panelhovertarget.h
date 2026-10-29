// SPDX-License-Identifier: MIT
#pragma once
#include <QList>
#include <QString>
#include <QWidget>
namespace Unbound {
struct PanelHoverSurface { int index; QString id; QWidget *surface; };
// Choose the closest owning panel, not the first containing panel in inventory
// order. Nested Mixer content must win over its enclosing Timeline surface.
inline int panelHoverTarget(QWidget *hovered,const QList<PanelHoverSurface> &surfaces) {
    for(auto ancestor=hovered;ancestor;ancestor=ancestor->parentWidget())
        for(const auto &candidate:surfaces)
            if(candidate.surface && candidate.surface->isVisible() &&
                (candidate.surface==ancestor || ancestor->property("groupPanel").toString()==candidate.id))
                return candidate.index;
    return -1;
}
}
