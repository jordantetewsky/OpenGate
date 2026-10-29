// SPDX-License-Identifier: MIT
#pragma once
#include "panelscalecontrols.h"
#include <QMenu>
#include <QJsonArray>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <functional>
#include "diagnostics.h"
#include "uiworktiming.h"

namespace Unbound {
class PanelMenus : public QObject
{
public:
    struct Panel {
        QString name;
        std::function<bool()> floating;
        std::function<void()> toggle;
        std::function<void()> dockNative = {};
        std::function<bool()> canDockNative = {};
        std::function<double()> interfaceScale = {};
        std::function<void(double)> setInterfaceScale = {};
    };
    explicit PanelMenus(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~PanelMenus() override { qApp->removeEventFilter(this); }
    void add(Panel panel) { panels.append(std::move(panel)); }
    void wire()
    {
        UiWorkTiming timing("panel.menus.wire",nullptr);
        QList<QPointer<QMenu>> menus;
        for (auto widget : QApplication::allWidgets()) if (auto menu = qobject_cast<QMenu *>(widget)) {
            auto button = qobject_cast<QWidget *>(menu->parent());
            if (optionsButton(button)) menus.append(menu);
        }
        if (menus.isEmpty()) return;
        // One synchronous inventory per pass, never a cache across native events.
        const auto owners=attachedOwners();
        for (auto menu:menus) if (menu) prepareMenu(menu,&owners);
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() != QEvent::Show) return false;
        auto menu = qobject_cast<QMenu *>(object);
        if (!menu || menu->property("unboundOwnMenu").toBool()) return false;
        prepareMenu(menu);
        return false;
    }
private:
    void prepareMenu(QMenu *menu,const QHash<QMenu *,int> *owners=nullptr)
    {
        if (!wiredMenus.contains(menu)) {
            wiredMenus.insert(menu);
            connect(menu, &QObject::destroyed, this, [this, menu] { wiredMenus.remove(menu); });
            connect(menu, &QMenu::aboutToShow, this, [this, menu] { prepareMenu(menu); });
        }
        const int index = ownerOf(menu,owners ? *owners : attachedOwners());
        if (index < 0) {
            for (auto command : menu->actions()) if (command->objectName() == QLatin1String("unboundDockPanel") ||
                command->objectName()==QLatin1String("unboundDockNativeWorkspace")) command->setVisible(false);
            QJsonArray parents;
            for (QObject *parent = menu->parent(); parent && parents.size() < 8; parent = parent->parent())
                parents.append(QJsonObject{{"name", parent->objectName()}, {"class", parent->metaObject()->className()}});
            logEvent(QStringLiteral("panel.menu.unmatched"), {{"name", menu->objectName()}, {"parents", parents}});
            return;
        }
        auto &panel = panels[index];
        QAction *command = nullptr;
        for (auto action : menu->actions()) if (action->objectName() == QLatin1String("unboundDockPanel")) command = action;
        if (!command) {
            menu->addSeparator();
            command = menu->addAction(QString());
            command->setObjectName(QStringLiteral("unboundDockPanel"));
        }
        if (!wired.contains(command)) {
            wired.insert(command);
            connect(command, &QObject::destroyed, this, [this, command] { wired.remove(command); });
            connect(command, &QAction::triggered, this, [this, command] {
                const int index = command->property("unboundPanelIndex").toInt();
                // Let Resolve finish closing its menu before changing widget ownership.
                QTimer::singleShot(0, this, [this, index] {
                    if (index < 0 || index >= panels.size()) return;
                    logEvent(QStringLiteral("panel.menu.toggle"), {{"panel", panels[index].name}});
                    panels[index].toggle();
                });
            });
        }
        command->setProperty("unboundPanelIndex", index);
        command->setVisible(true);
        command->setText(panel.floating() ? QStringLiteral("Dock Panel") : QStringLiteral("Undock Panel"));
        QAction *native=nullptr;
        for (auto action : menu->actions()) if (action->objectName()==QLatin1String("unboundDockNativeWorkspace")) native=action;
        if (panel.dockNative && !native) {
            native=menu->addAction(QStringLiteral("Dock in Native Workspace")); native->setObjectName("unboundDockNativeWorkspace");
        }
        if (native && !wired.contains(native)) {
            wired.insert(native);
            connect(native,&QObject::destroyed,this,[this,native] { wired.remove(native); });
            connect(native,&QAction::triggered,this,[this,native] {
                const int index=native->property("unboundPanelIndex").toInt();
                QTimer::singleShot(0,this,[this,index] {
                    if (index<0 || index>=panels.size() || !panels[index].dockNative) return;
                    logEvent("panel.menu.dock-native",{{"panel",panels[index].name}}); panels[index].dockNative();
                });
            });
        }
        if (native) {
            native->setProperty("unboundPanelIndex",index);
            native->setVisible(panel.dockNative && (!panel.canDockNative || panel.canDockNative()));
        }
        QMenu *scaleMenu=nullptr;
        for (auto action:menu->actions()) if (action->objectName()=="openGateScaleMenu") scaleMenu=action->menu();
        if (panel.interfaceScale && panel.setInterfaceScale && !scaleMenu) {
            scaleMenu=menu->addMenu("Panel Interface Size (Experimental)"); scaleMenu->menuAction()->setObjectName("openGateScaleMenu");
            scaleMenu->setProperty("unboundOwnMenu",true);
        }
        if (scaleMenu) {
            scaleMenu->clear(); scaleMenu->setEnabled(panel.canDockNative && panel.canDockNative());
            populatePanelScaleMenu(scaleMenu,this,panel.interfaceScale,panel.setInterfaceScale);
        }
        logEvent(QStringLiteral("panel.menu.ready"), {{"panel", panel.name}, {"floating", panel.floating()}});
    }
    static bool optionsButton(QWidget *button)
    {
        if (!button) return false;
        const QString name = button->objectName();
        if (name == "m_BtnMenu") return button->parentWidget() && button->parentWidget()->objectName() == "frameHeaderTitle";
        return name == "btnMediaPoolOptions" || name == "btnTitleOptions" || name == "btnAudioMixerOptions" ||
            name == "btnOptions" || name == "m_BtnDisplayOptions" || name == "btnEdlOptions" ||
            name == "tbtnPlayerOptions" || name == "metadataSelectionMenuBtn" || name == "btnCurveSettingsOptions" ||
            name == "btnEditToolbarPreferences";
    }
    int closestPanel(QWidget *button) const
    {
        for (auto parent = button->parentWidget(); parent; parent = parent->parentWidget())
            for (int i = 0; i < panels.size(); ++i) {
                // Timeline display preferences describe the timeline, not its detachable tools row.
                if (button->objectName() == "btnEditToolbarPreferences" && panels[i].name == "frameEditToolbarTop") continue;
                if (parent->objectName() == panels[i].name ||
                    parent->objectName() == QStringLiteral("unboundFloating_") + panels[i].name ||
                    parent->objectName() == QStringLiteral("unboundGroupCell_") + panels[i].name) return i;
            }
        return -1;
    }
    QHash<QMenu *,int> attachedOwners() const
    {
        QHash<QMenu *,int> owners;
        // Keep every attached owner: a known parent does not excuse ambiguity.
        for (auto window : QApplication::topLevelWidgets()) {
            for (auto button : window->findChildren<QAbstractButton *>()) {
                if (!optionsButton(button)) continue;
                QMenu *attached = nullptr;
                if (auto tool = qobject_cast<QToolButton *>(button)) attached = tool->menu();
                if (auto push = qobject_cast<QPushButton *>(button)) attached = push->menu();
                if (!attached) continue;
                const int index = closestPanel(button);
                if (index < 0) continue;
                const auto previous=owners.constFind(attached);
                // -2 remains ambiguous even if a later button repeats an owner.
                owners[attached]=previous!=owners.cend() && previous.value()!=index ? -2 : index;
            }
        }
        return owners;
    }
    int ownerOf(QMenu *menu,const QHash<QMenu *,int> &owners) const
    {
        // Resolve custom menus may be parented without QPushButton::menu().
        int result = -1;
        for (auto parent = menu->parent(); parent; parent = parent->parent())
            if (auto widget = qobject_cast<QWidget *>(parent))
                if (optionsButton(widget)) { result = closestPanel(widget); break; }
        const int attached=owners.value(menu,-1);
        if (attached==-2 || (result>=0 && attached>=0 && result!=attached)) return -1;
        return attached>=0 ? attached : result;
    }
    QList<Panel> panels;
    QSet<QAction *> wired;
    QSet<QMenu *> wiredMenus;
};
}
