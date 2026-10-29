// SPDX-License-Identifier: MIT
#pragma once
#include <QComboBox>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "arrangementstore.h"
#include "resolvelayout.h"
#include "panelgroup.h"
#include "arrangementorder.h"
#include "pageworkspacestate.h"
namespace Unbound {
class ArrangementControls : public QObject
{
public:
    ArrangementControls(QWidget *owner, QVBoxLayout *layout, const QString &selected, const QString &page = "edit") : QObject(owner), owner(owner), activePage(page)
    {
        auto row = new QHBoxLayout;
        presets = new QComboBox(owner); presets->setObjectName("arrangements"); presets->setMinimumWidth(0);
        presets->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); row->addWidget(presets,1);
        auto restore = new QPushButton("Restore", owner); restore->setObjectName("restoreArrangement"); row->addWidget(restore);
        auto manage = new QPushButton("Manage", owner); auto menu = new QMenu(manage); manage->setMenu(menu); row->addWidget(manage);
        for (const auto &operation : {"Save As…", "Update", "Rename…", "Duplicate…", "Delete", "Reorder"})
            connect(menu->addAction(QString::fromUtf8(operation)), &QAction::triggered, this,
                [this, operation] { manageArrangement(QString::fromUtf8(operation)); });
        layout->addLayout(row);
        state = new QLabel(owner); state->setObjectName("arrangementState"); state->setWordWrap(true); layout->addWidget(state);
        order = new ArrangementOrder(owner); layout->addWidget(order); order->hide();
        order->reordered = [this](const QStringList &ids) {
            const auto id = presets->currentData().toString();
            const auto error = arrangements.reorderPage(activePage,ids); reloadPresets(id);
            if (changed) changed();
            if (report) report(error.isEmpty() ? QStringLiteral("Workspace preset order saved.") : error);
            logEvent("arrangement.reorder", {{"success", error.isEmpty()}, {"error", error}});
        };
        reloadPresets(selected);
        connect(presets, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { if (workspace) updateState(workspace()); });
        connect(restore, &QPushButton::clicked, this, [this] { if (restoreRequested) restoreRequested(); });
    }
    QComboBox *selection() const { return presets; }
    QString page() const { return activePage; }
    void setPage(const QString &page)
    {
        if (page.isEmpty() || page == activePage) return;
        selections.insert(activePage, presets->currentData().toString());
        activePage = page;
        reloadPresets(selections.value(page));
    }
    QJsonObject find(const QString &id) const { return id==WorkspacePresetPolicy::resolveLayoutId(activePage) ? resolveLayoutEntry(activePage) : arrangements.find(id); }
    QJsonArray list() const { auto result=arrangements.visibleList();const auto builtin=resolveLayoutEntry(activePage);if(!builtin.isEmpty())result.prepend(builtin);return result; }
    bool hasHidden() const {return arrangements.hasHidden();}
    void invoke(const QString &operation, const QString &id = {})
    {
        if(operation=="Show Hidden Arrangements") {
            const auto selected=presets->currentData().toString();const auto error=arrangements.revealHidden();
            if(error.isEmpty()){reloadPresets(selected);if(changed)changed();}
            if(report)report(error.isEmpty()?QStringLiteral("Hidden arrangements are visible again; the current layout was not changed."):error);
            logEvent("arrangement.hidden.reveal",{{"success",error.isEmpty()},{"error",error}});return;
        }
        if (!id.isEmpty()) {
            const int index = presets->findData(id);
            if (index < 0 || workspacePage(find(id)) != activePage || find(id).isEmpty()) { if (report) report("Arrangement no longer exists."); return; }
            if (operation == "Restore") { const QSignalBlocker blocked(presets);presets->setCurrentIndex(index); }
        }
        if (operation == "Restore") { if (restoreRequested) restoreRequested(); }
        else manageArrangement(operation, id);
    }
    std::function<QJsonObject()> workspace;
    std::function<void()> changed, restoreRequested;
    std::function<void(QString)> report;
    void updateState(const QJsonObject &current)
    {
        const auto saved = find(presets->currentData().toString());
        state->setText(!arrangements.error().isEmpty() ? arrangements.error() : saved.isEmpty() ?
            QStringLiteral("Current layout is not saved as a preset.") :
            normalizedArrangement(saved["workspace"].toObject()) == normalizedArrangement(current) ?
            QStringLiteral("Matches saved arrangement") : QStringLiteral("Modified — use Update to save changes"));
    }
private:
    static QJsonObject normalizedArrangement(QJsonObject value)
    {
        value["layout_mode"] = PageWorkspaceState::mode(value);
        const bool manual=value["background_policy"]=="manual-hidden";
        value["background_policy"]=manual?"manual-hidden":"automatic";
        value["hide_background"]=manual;
        value["native_workspace_mode"]=manual?"floating":"native";
        value["snap_windows"] = value.value("snap_windows").toBool(true);
        value["page"] = value.value("page").toString("edit");
        QJsonArray layouts;
        for (auto saved : value.value("groups").toArray()) {
            auto group = saved.toObject(); const auto tree = Unbound::PanelGroup::savedTree(group);
            QStringList names; Unbound::groupTreeNames(tree, names);
            group["pinned"] = group.value("pinned").toBool();
            group["hidden"] = group.value("hidden").toBool();
            group["tree"] = tree; group["panels"] = QJsonArray::fromStringList(names);
            group.remove("orientation"); group.remove("sizes"); layouts.append(group);
        }
        value["groups"] = layouts;
        auto savedPanels = value["panels"].toObject();
        for (auto entry = savedPanels.begin(); entry != savedPanels.end(); ++entry) {
            auto panel = entry.value().toObject(); panel["pinned"] = panel.value("pinned").toBool(); entry.value() = panel;
        }
        value["panels"] = savedPanels; return value;
    }
    void reloadPresets(const QString &selected = {})
    {
        const QSignalBlocker blocker(presets);
        presets->clear(); presets->addItem(QStringLiteral("Unsaved arrangement"), QString());
        const auto builtin=resolveLayoutEntry(activePage);if(!builtin.isEmpty())presets->addItem(builtin["name"].toString(),builtin["id"].toString());
        for (const auto &value : arrangements.visibleList(activePage)) {
            const auto entry = value.toObject();
            presets->addItem(entry["name"].toString(), entry["id"].toString());
        }
        presets->setCurrentIndex(qMax(0, presets->findData(selected)));
        if (order) order->reload(arrangements.visibleList(activePage));
    }
    void manageArrangement(const QString &operation, const QString &target = {})
    {
        const auto operationPage = activePage;
        logEvent(QStringLiteral("button.click"), {{"action", QStringLiteral("arrangement-") + operation}});
        if (operation == "Reorder") { order->setVisible(order->isHidden()); return; }
        if (changed) changed();
        const auto previous = presets->currentData().toString();
        QString id = target.isEmpty() ? previous : target, error;
        const auto entry = find(id);
        if (WorkspacePresetPolicy::builtIn(id) && operation != QString::fromUtf8("Save As…") && operation != QString::fromUtf8("Duplicate…")) { if(report)report("Resolve Layout is read-only. Save as a new workspace instead.");return; }
        if (operation != QString::fromUtf8("Save As…") && (entry.isEmpty() || workspacePage(entry) != operationPage)) {
            report(QStringLiteral("Select a saved arrangement first.")); return;
        }
        const auto capture = [this, operationPage]() {
            const auto value = workspace ? workspace() : QJsonObject{};
            return activePage == operationPage && value.value("page").toString("edit") == operationPage ? value : QJsonObject{};
        };
        if (operation == "Update") {
            const auto value = capture();
            error = value.isEmpty() ? "Workspace page changed; no preset was updated." : arrangements.update(id, value);
        }
        else if (operation == "Delete") {
            if (QMessageBox::question(owner, QStringLiteral("Delete arrangement"),
                QStringLiteral("Delete “%1”? Your current windows will stay in place.").arg(entry["name"].toString())) != QMessageBox::Yes) return;
            if (activePage != operationPage) { if (report) report("Workspace page changed; no preset was deleted."); return; }
            error = arrangements.remove(id); if (error.isEmpty()) id.clear();
        } else {
            bool accepted = false;
            QString initial = entry["name"].toString();
            if (operation == QString::fromUtf8("Duplicate…")) initial += QStringLiteral(" copy");
            const auto name = QInputDialog::getText(owner, operation, QStringLiteral("Arrangement name"), QLineEdit::Normal, initial, &accepted);
            if (!accepted) return;
            if (activePage != operationPage) { if (report) report("Workspace page changed; no preset was saved."); return; }
            if (operation == QString::fromUtf8("Rename…")) error = arrangements.rename(id, name);
            else {
                const auto value = operation == QString::fromUtf8("Duplicate…") ? entry["workspace"].toObject() : capture();
                error = value.isEmpty() ? "Workspace page changed; no preset was saved." : arrangements.create(name, value, id);
            }
        }
        const auto operatedId = entry.isEmpty() ? id : entry["id"].toString();
        if ((operation == QString::fromUtf8("Rename…") || operation == "Delete") && !target.isEmpty() && previous != target)
            id = previous;
        reloadPresets(id); if (changed) changed();
        report(error.isEmpty() ? QStringLiteral("Arrangement saved.") : error);
        Unbound::logEvent(QStringLiteral("arrangement.result"), {{"operation", operation}, {"id", operatedId}, {"selected", id}, {"success", error.isEmpty()}, {"error", error}});
    }
    QWidget *owner;
    QString activePage;
    QHash<QString, QString> selections;
    QComboBox *presets = nullptr;
    QLabel *state = nullptr;
    ArrangementStore arrangements;
    ArrangementOrder *order = nullptr;
};
}
