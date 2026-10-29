// SPDX-License-Identifier: MIT
#pragma once
#include <QHash>
#include <QPointer>
#include <QWidget>
namespace Unbound {
// Visibility only for native surfaces without a Resolve panel-toggle button.
// Never reparents, changes constraints, creates a host or loads an arrangement.
class NativeSurfaceDoors {
public:
    bool set(const QString &id,QWidget *surface,bool visible) {
        if(!surface || surface->isWindow())return false;
        auto &entry=entries[id];
        if(entry.widget!=surface)entry={surface,surface->parentWidget(),surface->isHidden()};
        // Showing a child beneath a hidden ancestor cannot satisfy Show. Leave
        // that ancestor to its native owner instead of reporting false success.
        if(visible && !surface->parentWidget()->isVisible())return false;
        surface->setVisible(visible);return surface->isHidden()!=visible;
    }
    bool contains(const QString &id) const {return entries.contains(id) && entries[id].widget;}
    bool modified(const QString &id) const {
        const auto entry=entries.value(id);return entry.widget && entry.widget->isHidden()!=entry.hidden;
    }
    bool hidden(const QString &id) const {return entries.value(id).widget && entries.value(id).widget->isHidden();}
    QString restore() {
        for(auto it=entries.begin();it!=entries.end();++it)if(it->widget) {
            if(!it->parent || it->widget->parentWidget()!=it->parent)
                return QStringLiteral("Native visibility owner changed: ")+it.key();
        }
        // Validate every owner before changing any visibility: a failed Native
        // return must not leave half the tracked surfaces restored.
        for(auto it=entries.begin();it!=entries.end();++it)
            if(it->widget)it->widget->setVisible(!it->hidden);
        entries.clear();return {};
    }
private:
    struct Entry {QPointer<QWidget> widget,parent;bool hidden=false;};
    QHash<QString,Entry> entries;
};
}
