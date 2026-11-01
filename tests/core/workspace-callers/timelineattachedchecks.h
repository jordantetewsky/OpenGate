// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelsession.h"
#include <QEventLoop>
#include <QTimer>
#include "timelineviewportfixtures.h"

namespace OpenGateTest {
inline void settledTimelineBoundary() {
    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();
}
template<class Check> void timelineAttachedChecks(Check check) {
    qputenv("UNBOUND_TEST_HIDDEN_HOSTS", "1");
    QWidget page;
    page.setAttribute(Qt::WA_DontShowOnScreen);
    auto pageLayout = new QVBoxLayout(&page);
    QWidget timeline;
    timeline.setObjectName("frameConformTimelines");
    pageLayout->addWidget(&timeline);
    auto layout = new QVBoxLayout(&timeline);
    auto toolbar = new QPushButton("Native tools");
    toolbar->setObjectName("frameEditToolbarTop"); toolbar->setFixedHeight(31); layout->addWidget(toolbar);
    auto header = new QWidget;
    header->setObjectName("frameHeaderContainer"); header->setFixedHeight(28); layout->addWidget(header);
    auto headerLayout = new QVBoxLayout(header); headerLayout->setContentsMargins(0, 0, 0, 0);
    auto tabs = new QWidget;
    tabs->setObjectName("timelineTabsHolder"); headerLayout->addWidget(tabs);
    QGraphicsScene scene;
    scene.addRect(0, 0, 700, 300);
    auto view = new QGraphicsView(&scene);
    view->setObjectName("m_TiTimelineView"); layout->addWidget(view);
    auto choice = new QAction(toolbar);
    choice->setText("Display Stacked Timelines"); choice->setCheckable(true); choice->setChecked(true);
    int nativeInvocations = 0;
    QObject::connect(choice, &QAction::triggered, toolbar, [&] { ++nativeInvocations; header->setVisible(choice->isChecked()); });
    page.resize(900, 600); page.show(); TimelineViewportFixture::settle();
    Unbound::ManagedPanelSession session;
    check(session.undock(&timeline, &page, "Timeline").isEmpty(), "assembled Timeline entry leases toolbar, content and native tabs together");
    settledTimelineBoundary();
    page.hide(); header->hide(); toolbar->hide(); toolbar->setFixedHeight(0);
    page.show(); settledTimelineBoundary();
    check(!toolbar->isHidden() && toolbar->height() == 31 && !header->isHidden() && !tabs->isHidden(),
          "page return restores explicitly hidden toolbar and native tab ancestor, including collapsed height");
    check(choice->isChecked() && nativeInvocations == 0,
          "page recovery preserves Display Stacked Timelines instead of triggering or resetting it");
    choice->trigger(); settledTimelineBoundary();
    check(header->isHidden() && !choice->isChecked(), "explicit native user choice to hide stacked tabs is retained");
    page.hide(); toolbar->hide(); page.show(); settledTimelineBoundary();
    check(header->isHidden() && !choice->isChecked() && !toolbar->isHidden() && nativeInvocations == 1,
          "page return repairs toolbar while respecting disabled stacked tabs");
    const auto saved = session.interfaceState();
    check(saved["timeline_toolbar_visible"].toBool() && !saved["timeline_tabs_visible"].toBool(true),
          "existing session state serializes independent toolbar and tab intentions");
    session.setHiddenByUser(true, false);
    page.hide(); page.show(); settledTimelineBoundary();
    check(session.hiddenByUser() && !session.window()->isVisible(), "header reconciliation never reveals a user-hidden Timeline host");
    session.setHiddenByUser(false, false);
    check(session.dock().isEmpty(), "native return releases toolbar/content presentation without a layout reset");
    TimelineViewportFixture::settle();
    check(toolbar->parentWidget() == &timeline && layout->indexOf(toolbar) == 0 && nativeInvocations == 1,
          "native toolbar owner and layout position restore exactly, with native choice unchanged");
    Unbound::ManagedPanelSession restored;
    check(restored.restoreInterfaceState(saved) && restored.undock(&timeline, &page, "Timeline").isEmpty(),
          "fresh session restores the same working presentation state");
    settledTimelineBoundary();
    check(!toolbar->isHidden() && header->isHidden() && restored.interfaceState()["timeline_tabs_visible"] == saved["timeline_tabs_visible"],
          "page/restart state preserves toolbar and stacked-tab intention independently");
    check(restored.dock().isEmpty(), "restored session returns all native content cleanly");
}
}
