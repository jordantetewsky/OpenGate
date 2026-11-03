// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colormanagerwidget.h"
#include "../../../src/core/panelcallermenu.h"
#include "../../../src/core/panelcontrols.h"
namespace OpenGateTest {
// Extend the existing assembled entry fixture without copying its native tree.
template <class Check, class Settle>
void colorManagerEntryChecks(PanelControls &controls, QMenu *ui, Unbound::PanelCallerMenu *registry,
                             QWidget *main, QWidget *page, QWidget *timeline, QWidget *source,
                             QWidget *program, Check check, Settle settle) {
    auto action = ui->findChild<QAction *>("openGateShowColorManager"),
         native = ui->findChild<QAction *>("unboundRestoreNativeWorkspace");
    check(action && native && ui->actions().indexOf(action) + 1 == ui->actions().indexOf(native),
          "color manager entry is above native return in assembled menu");
    if (!action || !native)
        return;
    action->trigger();
    settle(600);
    auto content = main->findChild<QWidget *>("openGateColorManagerPanel");
    check(content && dynamic_cast<Unbound::ColorManagerWidget *>(content) &&
              content->window() != main,
          "assembled menu opens real Qt manager in the existing floating host");
    check(source->parentWidget() == page && program->parentWidget() == page &&
              timeline->parentWidget() == page,
          "opening owned manager preserves native media panel owners");
    if (!content)
        return;
    auto host = content->window();
    const auto frame = host->geometry();
    host->close();
    settle(200);
    check(!content->isVisible(), "closing owned manager hides retained host");
    QMenu flat;
    registry->populateFlat(&flat);
    QAction *door = nullptr;
    for (auto item : flat.actions())
        if (item->text() == "Clips & Tracks Color Manager")
            door = item;
    check(door && door->menu() == nullptr, "Color Manager is one flat Panels entry");
    if (door)
        door->trigger();
    settle(350);
    check(content->window() == host && host->geometry() == frame && content->isVisible(),
          "Panels recalls same manager and geometry");
    auto floating = dynamic_cast<Unbound::WorkspaceFloatingHost *>(host);
    check(floating && bool(floating->attachRequested),
          "manager uses existing panel attach gesture");
    if (floating && floating->attachRequested)
        floating->attachRequested(timeline, Qt::RightEdge);
    settle(450);
    check(content->window() == main && timeline->window() == main,
          "manager docks beside Timeline in existing arrangement");
    native->trigger();
    settle(500);
    check(content->isHidden() && timeline->parentWidget() == page &&
              source->parentWidget() == page && program->parentWidget() == page,
          "Original UI parks owned manager and restores native panel slots");
    timeline->setFocusPolicy(Qt::StrongFocus);
    QApplication::setActiveWindow(main);
    timeline->setFocus();
    settle(50);
    check(controls.invokeWorkflowCommand("color-manager.open"),
          "mapped command reaches shared manager reveal operation");
    settle(400);
    check(content->isVisible() &&
              main->findChildren<QWidget *>("openGateColorManagerPanel").size() == 1,
          "all entry points reuse one manager rather than duplicate palette controllers");
    native->trigger();
    settle(500);
}
} // namespace OpenGateTest
