// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QVariant>
#include <QWidget>
namespace Unbound {
inline void registerDockSurfaces(QWidget *host,const QList<QWidget *> &surfaces)
{
    QVariantList values;
    for (auto surface : surfaces) values.append(QVariant::fromValue(QPointer<QWidget>(surface)));
    host->setProperty("unboundDockSurfaces",values);
}
inline QList<QWidget *> dockSurfaces(QWidget *host)
{
    QList<QWidget *> result; const auto saved = host->property("unboundDockSurfaces");
    if (saved.isValid()) {
        for (const auto &entry : saved.toList()) {
            auto surface = entry.value<QPointer<QWidget>>();
            if (surface && surface->isVisible()) result.append(surface);
        }
    } else for (auto child : host->findChildren<QWidget *>())
        if (child->objectName().startsWith("unboundGroupCell_") && child->isVisible()) result.append(child);
    return result;
}
}
