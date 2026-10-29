// SPDX-License-Identifier: MIT
#pragma once
#include "productidentity.h"
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSet>
#include <QHash>
#include <QTimer>
#include <functional>
#include "diagnostics.h"
#include "workspacepages.h"
#include "workspacepresetpolicy.h"
#include "keyboardlauncher.h"
#include "workflowcommandmenu.h"

namespace Unbound {
// Native menus and the controls window dispatch to the same arrangement owner.
// The native recovery action is outside the removable preset collection.
// Show only arrangements belonging to the active page. Until a page has a verified
// panel backend, keep its saved-workspace operations disabled. Future cross-page
// panels must preserve their source-page lifecycle rather than bypass this gate.
class WorkspaceArrangements : public QObject
{
public:
    explicit WorkspaceArrangements(QObject *owner) : QObject(owner) { commandMenu=new WorkflowCommandMenu(this); qApp->installEventFilter(this); }
    ~WorkspaceArrangements() override { if (qApp) qApp->removeEventFilter(this); }
    std::function<QJsonArray()> entries;
    std::function<QString()> selected;
    std::function<void(QString, QString)> invoke;
    std::function<void(QString)> nativeWorkspace;
    std::function<void()> showPanelManager;
    std::function<void()> showPanelShortcuts;
    std::function<QMenu *(QMenu *)> panelMenu;
    std::function<QMenu *(QMenu *)> commandsMenu;
    std::function<QString()> currentPage = [] { return QStringLiteral("edit"); };
    std::function<bool()> nativeActive = [] { return false; };
    std::function<bool()> hiddenArrangements=[] {return false;};
    std::function<bool(QString)> supportedPage = [](const QString &page) { return page == "edit"; };
    void wire()
    {
        if (!entries || !selected || !invoke || !nativeWorkspace) return;
        bool retained=false;
        for(auto root:roots)if(root){refresh(root);retained=true;}
        if(retained)return;
        roots.clear();
        const auto widgets = QApplication::allWidgets();
        for (auto widget : widgets) if (auto workspace = qobject_cast<QMenu *>(widget)) {
            if (workspace->objectName() != QLatin1String("workspace")) continue;
            for (auto candidate : widgets) if (auto bar = qobject_cast<QMenuBar *>(candidate)) {
                const auto actions = bar->actions(); const int index = actions.indexOf(workspace->menuAction());
                if (index < 0) continue;
                auto menu = bar->findChild<QMenu *>("unboundWorkspaceUI",Qt::FindDirectChildrenOnly);
                if (!menu) {
                    menu = new QMenu("OpenGate UI",bar); menu->setObjectName("unboundWorkspaceUI");
                    menu->setProperty("unboundOwnMenu",true); menu->menuAction()->setMenuRole(QAction::NoRole);
                    logEvent("workspace.menu.attached",{{"placement","menubar"},{"label","OpenGate UI"},{"page",currentPage()}});
                }
                menu->setTitle("OpenGate UI");
                const auto current = bar->actions();
                if (current.indexOf(menu->menuAction()) != current.indexOf(workspace->menuAction())+1)
                    bar->insertMenu(index+1 < actions.size() ? actions[index+1] : nullptr,menu);
                roots.append(menu);watch(menu,menu); refresh(menu);
                break;
            }
        }
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        if (event->type() != QEvent::Show && event->type() != QEvent::Hide) return false;
        auto widget = qobject_cast<QWidget *>(object);
        if (widget && widget->objectName().startsWith("WStackPage_") && !pageRefreshQueued) {
            pageRefreshQueued = true;
            QTimer::singleShot(0,this,[this] { pageRefreshQueued = false; wire(); });
        }
        return false;
    }
private:
    QAction *action(QMenu *menu, const QString &name, const QString &label, const QString &operation, const QString &id = {},bool attach=true)
    {
        QAction *result = nullptr;
        result = menu->findChild<QAction *>(name,Qt::FindDirectChildrenOnly);
        if (!result) { result = new QAction(label,menu); result->setObjectName(name); }
        if (attach && !menu->actions().contains(result)) menu->addAction(result);
        if (result->text() != label) result->setText(label);
        result->setMenuRole(QAction::NoRole); result->setProperty("unboundArrangementId", id);
        result->setProperty("unboundWorkspacePage",currentPage());
        if (!wiredActions.contains(result)) {
            wiredActions.insert(result);
            connect(result, &QObject::destroyed, this, [this, result] { wiredActions.remove(result); });
            connect(result, &QAction::triggered, this, [this, result, operation] {
                const auto id = result->property("unboundArrangementId").toString();
                const auto page = result->property("unboundWorkspacePage").toString();
                QTimer::singleShot(0, this, [this, operation, id, page] {
                    if(operation=="Show Hidden Arrangements") {
                        logEvent("workspace.menu.command",{{"operation",operation},{"page",currentPage()}});
                        invoke(operation,{});wire();return;
                    }
                    if (operation == "PanelShortcuts") { if (showPanelShortcuts) showPanelShortcuts(); return; }
                    if (operation == "About") { QMessageBox::about(nullptr,"About OpenGate",productCredit()); return; }
                    if (operation == "Keyboard") { openNativeKeyboardCustomization(); return; }
                    if (operation == "PanelManager") {
                        logEvent("workspace.menu.command",{{"operation",operation},{"page",currentPage()}});
                        if (showPanelManager) showPanelManager();
                        return;
                    }
                    if (currentPage() != page) { logEvent("workspace.menu.cancelled",{{"reason","page-changed"},{"page",page}}); return; }
                    if (operation != "Native" && !supportedPage(page)) { logEvent("workspace.menu.cancelled",{{"reason","unsupported-page"},{"page",page}}); return; }
                    logEvent(QStringLiteral("workspace.menu.command"), {{"operation", operation}, {"id", id},{"page",page}});
                    if (operation == "Native") nativeWorkspace(page); else invoke(operation, id);
                    wire();
                });
            });
        }
        return result;
    }
    void watch(QMenu *menu, QMenu *root)
    {
        if (wiredMenus.contains(menu)) return;
        wiredMenus.insert(menu);
        connect(menu, &QObject::destroyed, this, [this, menu] { wiredMenus.remove(menu); rendered.remove(menu); });
        connect(menu, &QMenu::aboutToShow, this, [this, root] { refresh(root); });
    }
    void refresh(QMenu *root)
    {
        commandMenu->wire(root);
        const auto page = currentPage();
        auto callers=panelMenu ? panelMenu(root) : nullptr;
        auto commands=commandsMenu ? commandsMenu(root) : nullptr;
        auto native = action(root, "unboundRestoreNativeWorkspace",QStringLiteral("Use Original Resolve UI"), "Native");
        native->setToolTip("Restore Resolve’s standard interface without OpenGate layout management");
        auto about=action(root,"openGateAbout","About OpenGate…","About");
        auto shortcuts=action(root,"unboundPanelShortcuts",QString::fromUtf8(commands?"Keyboard Shortcuts…":"OpenGate Panel Shortcuts…"),"PanelShortcuts",{},!commands);
        if(commands && !commands->actions().contains(shortcuts))commands->addAction(shortcuts);
        shortcuts->setEnabled(bool(showPanelShortcuts));
        shortcuts->setToolTip("Open Resolve’s native Keyboard Customization, filtered to OpenGate commands; native presets and assignments are authoritative");
        auto manager = action(root,"unboundShowPanelManager",QString::fromUtf8("OpenGate Panel Manager…"),"PanelManager");
        manager->setEnabled(bool(showPanelManager));
        const bool nativeSelected = nativeActive(); native->setCheckable(true); native->setChecked(nativeSelected);
        auto menu = root->findChild<QMenu *>("unboundWorkspaceArrangements", Qt::FindDirectChildrenOnly);
        bool created = !menu;
        if (!menu) {
            menu = new QMenu("Manage Saved Workspaces", root); menu->setObjectName("unboundWorkspaceArrangements"); root->addMenu(menu);
        }
        menu->setTitle("Manage Saved Workspaces");
        menu->setProperty("unboundOwnMenu", true);
        watch(root, root); watch(menu, root);
        const auto all = entries(); QJsonArray saved; QSet<QString> allIds;
        for (const auto &value : all) allIds.insert(value.toObject()["id"].toString());
        for (const auto &value : all) if (workspacePage(value.toObject()) == page) saved.append(value);
        const auto current = selected(); const bool supported = supportedPage(page);
        QJsonArray labels;
        for (const auto &value : saved) {
            const auto entry = value.toObject(); labels.append(QJsonObject{{"id",entry["id"]},{"name",entry["name"]}});
        }
        const bool hidden=hiddenArrangements();
        auto advanced=root->findChild<QMenu *>("openGateAdvanced",Qt::FindDirectChildrenOnly);
        if(!advanced){advanced=new QMenu("Advanced",root);advanced->setObjectName("openGateAdvanced");advanced->setProperty("unboundOwnMenu",true);}
        advanced->menuAction()->setVisible(true);
        action(advanced,"openGateShowHiddenArrangements","Show Hidden Arrangements","Show Hidden Arrangements")->setEnabled(hidden);
        const QJsonObject model{{"hidden_arrangements",hidden},{"page",page},{"native",nativeSelected},{"supported",supported},{"selected",current},{"entries",labels}};
        if (!created && rendered.value(root) == model && (!callers || root->actions().contains(callers->menuAction())) &&
            (!commands || root->actions().contains(commands->menuAction()))) return;
        auto saveAs=action(menu, "unboundSaveWorkspaceAs", QString::fromUtf8("Save as New Workspace…"), QString::fromUtf8("Save As…"),{},false);saveAs->setEnabled(supported);
        menu->removeAction(saveAs);menu->menuAction()->setVisible(!saved.isEmpty());
        bool currentExists = false; QSet<QString> ids;
        for (const auto &value : saved) if (value.toObject()["id"].toString() == current && !value.toObject()["read_only"].toBool() && WorkspacePresetPolicy::writable(current)) currentExists = true;
        auto update=action(root, "unboundUpdateWorkspace", "Update Current Workspace", "Update", current);update->setEnabled(supported && currentExists && !nativeSelected);
        action(menu, "unboundReorderWorkspaces", QString::fromUtf8("Reorder Presets…"), "Reorder")->setEnabled(supported);
        auto browser=root->findChild<QMenu *>("openGateWorkspaces",Qt::FindDirectChildrenOnly);
        if(!browser){browser=new QMenu("OpenGate Workspaces",root);browser->setObjectName("openGateWorkspaces");browser->setProperty("unboundOwnMenu",true);}
        watch(browser,root);
        QList<QAction *> ordered, quick;
        for (const auto &value : saved) {
            const auto entry = value.toObject(); const auto id = entry["id"].toString(); ids.insert(id);
            auto quickRestore = action(browser,"unboundWorkspaceRestore_"+id,entry["name"].toString().replace('&',"&&"),"Restore",id);
            quickRestore->setCheckable(true); quickRestore->setChecked(!nativeSelected && id == current); quickRestore->setEnabled(supported); quick.append(quickRestore);
            if(entry["read_only"].toBool())continue;
            auto preset = menu->findChild<QMenu *>("unboundWorkspace_" + id, Qt::FindDirectChildrenOnly);
            if (!preset) {
                preset = new QMenu(menu); preset->setObjectName("unboundWorkspace_" + id);
                preset->setProperty("unboundArrangementId", id); menu->addMenu(preset);
            } else if (!menu->actions().contains(preset->menuAction())) menu->addMenu(preset);
            preset->setProperty("unboundOwnMenu", true);
            preset->menuAction()->setEnabled(supported);
            auto title = entry["name"].toString(); title.replace('&', "&&"); preset->setTitle(title);
            preset->menuAction()->setCheckable(true); preset->menuAction()->setChecked(!nativeSelected && id == current);
            ordered.append(preset->menuAction());
            watch(preset, root);
            action(preset, "unboundRestoreArrangement", "Restore Workspace", "Restore", id);
            action(preset, "unboundUpdateArrangement", "Update from Current Layout", "Update", id);
            for (const auto &operation : {QString::fromUtf8("Rename…"), QString::fromUtf8("Duplicate…"), QStringLiteral("Delete")})
                action(preset, "unboundArrangement_" + operation, operation, operation, id);
        }
        for (auto preset : menu->findChildren<QMenu *>(QString(), Qt::FindDirectChildrenOnly))
            if (!ids.contains(preset->property("unboundArrangementId").toString())) {
                menu->removeAction(preset->menuAction());
                if (!allIds.contains(preset->property("unboundArrangementId").toString())) {
                    preset->setObjectName("unboundRetiredWorkspace"); preset->deleteLater();
                }
            }
        if(browser->actions()!=quick){for(auto item:browser->actions())browser->removeAction(item);for(auto item:quick)browser->addAction(item);}
        QList<QAction *> currentOrder;
        for (auto item : menu->actions()) if (item->menu()) currentOrder.append(item);
        if (currentOrder != ordered) {
            for (auto item : currentOrder) menu->removeAction(item);
            for (auto item : ordered) menu->addAction(item);
        }
        auto divider = root->findChild<QAction *>("unboundSavedWorkspaceSeparator",Qt::FindDirectChildrenOnly);
        if (!divider) { divider = root->addSeparator(); divider->setObjectName("unboundSavedWorkspaceSeparator"); }
        auto manageDivider = root->findChild<QAction *>("unboundWorkspaceManagementSeparator",Qt::FindDirectChildrenOnly);
        if (!manageDivider) { manageDivider = root->addSeparator(); manageDivider->setObjectName("unboundWorkspaceManagementSeparator"); }
        if(!advanced->actions().contains(menu->menuAction()))advanced->addMenu(menu);
        QList<QAction *> topOrder{browser->menuAction(),update,saveAs,divider,native,manageDivider};
        if (callers) topOrder.append(callers->menuAction());topOrder.append(manager);topOrder.append(commands?commands->menuAction():shortcuts);topOrder.append(advanced->menuAction());topOrder.append(about);
        divider->setVisible(true);
        if (root->actions() != topOrder) { for (auto item : root->actions()) root->removeAction(item); for (auto item : topOrder) root->addAction(item); }
        for (auto item : root->findChildren<QAction *>(QString(),Qt::FindDirectChildrenOnly))
            if (item->objectName().startsWith("unboundWorkspaceRestore_") && !allIds.contains(item->property("unboundArrangementId").toString())) {
                item->setObjectName("unboundRetiredWorkspaceAction"); item->deleteLater();
            }
        rendered[root] = model;
    }
    QSet<QAction *> wiredActions;
    QList<QPointer<QMenu>> roots;
    WorkflowCommandMenu *commandMenu=nullptr;
    QSet<QMenu *> wiredMenus;
    QHash<QMenu *,QJsonObject> rendered;
    bool pageRefreshQueued = false;
};
}
