// SPDX-License-Identifier: MIT
#pragma once
#include "clipcolorstock.h"
#include "isolatednativeaction.h"
#include "menutrackingcommand.h"
#include "timelineclipcolorpreview.h"
#include <QMenu>
#include <QShortcutEvent>

namespace Unbound {
// Route candidate stock/Clear menu selections and QAction shortcut events into
// the same native assignment transaction. Native swatch definitions are intact;
// original menu actions are leased and restored, never disconnected or replayed.
class ClipColorNativeCommands : public QObject {
  public:
    using Resolve = std::function<TimelineClipColorPreview *(QWidget *)>;
    ClipColorNativeCommands(QObject *owner, Resolve resolve) : QObject(owner), resolve(std::move(resolve)) {
        qApp->installEventFilter(this);
    }
    ~ClipColorNativeCommands() override {
        if (qApp) qApp->removeEventFilter(this);
        for (auto it = rows.begin(); it != rows.end(); ++it)
            if (it.key()) restore(it.key(), it.value());
    }
    void prepare(QMenu *menu, QWidget *origin, TimelineClipColorPreview *target) {
        if (!target || !target->persistent()) {
            if (rows.contains(menu)) restore(menu, rows.take(menu));
            origins.remove(menu);
            return;
        }
        if (!rows.contains(menu)) {
            rows.insert(menu, {});
            connect(menu, &QObject::destroyed, this, [this, menu] {
                const auto lease = rows.take(menu);
                origins.remove(menu);
                for (const auto &row : lease) if (row.mirror) delete row.mirror;
            });
        }
        origins.insert(menu, origin);
        auto &lease = rows[menu];
        for (int i = lease.size()-1; i >= 0; --i)
            if (!lease[i].original || !lease[i].mirror || !menu->actions().contains(lease[i].mirror)) {
                if (lease[i].mirror) delete lease[i].mirror;
                lease.removeAt(i);
            }
        for (auto action : menu->actions()) {
            if (action->property("openGateCommandId").isValid()) continue;
            QString name;
            if (!stockName(action, name)) continue;
            Row row;
            row.original = action;
            row.mirror = IsolatedNativeAction::create(action->text(), this,
                [this, weakMenu = QPointer<QMenu>(menu), name] {
                    const auto weak = weakMenu ? origins.value(weakMenu) : QPointer<QWidget>();
                    MenuTrackingCommand::queue(this, [this, weak, name] {
                        auto target = weak && resolve ? resolve(weak) : nullptr;
                        if (target && target->persistent()) target->assignNative(name);
                    });
                });
            row.mirror->setObjectName("openGateNativeClipColor_" + (name.isEmpty() ? "Default" : name));
            row.mirror->setProperty("openGateCommandId", "clip-color.timeline.native." + name);
            row.mirror->setIcon(action->icon());
            row.mirror->setCheckable(action->isCheckable());
            row.mirror->setChecked(action->isChecked() && target->selectedPresetId().isEmpty());
            row.mirror->setEnabled(action->isEnabled() && target->hasSelection());
            menu->insertAction(action, row.mirror);
            menu->removeAction(action);
            lease.append(row);
        }
        for (const auto &row : lease)
            if (row.original && row.mirror) {
                row.mirror->setChecked(row.original->isChecked() && target->selectedPresetId().isEmpty());
                row.mirror->setEnabled(row.original->isEnabled() && target->hasSelection());
            }
    }
  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() != QEvent::Shortcut) return false;
        auto action = qobject_cast<QAction *>(object);
        auto shortcut = static_cast<QShortcutEvent *>(event);
        QString name;
        if (!action || shortcut->isAmbiguous() || action->property("openGateCommandId").isValid() ||
            !action->objectName().startsWith("markSetClipColor") ||
            !stockName(action, name)) return false;
        auto focus = QApplication::focusWidget();
        if (!focus || textEditor(focus)) return false;
        auto target = resolve ? resolve(focus) : nullptr;
        if (!target || !target->persistent()) return false;
        // Consume only actual scoped stock-color shortcuts. Arbitrary native
        // QAction::trigger calls are observed, not intercepted by this boundary.
        target->assignNative(name);
        event->accept();
        return true;
    }
  private:
    struct Row { QPointer<QAction> original, mirror; };
    QHash<QMenu *, QVector<Row>> rows;
    QHash<QMenu *, QPointer<QWidget>> origins;
    Resolve resolve;
    static bool stockName(QAction *action, QString &name) {
        const auto id = action->objectName();
        const QString prefix = "markSetClipColor";
        if (id.startsWith(prefix)) name = id.mid(prefix.size());
        else { name = action->text(); name.remove('&'); name = name.trimmed(); }
        if (name == "Default" || name == "Clear Color") { name.clear(); return true; }
        for (const auto &stock : clipColorStockReferences()) if (stock.first == name) return true;
        return false;
    }
    static void restore(QMenu *menu, const QVector<Row> &lease) {
        for (const auto &row : lease) {
            if (row.original && row.mirror && menu->actions().contains(row.mirror))
                menu->insertAction(row.mirror, row.original);
            if (row.mirror) delete row.mirror;
        }
    }
};
} // namespace Unbound
