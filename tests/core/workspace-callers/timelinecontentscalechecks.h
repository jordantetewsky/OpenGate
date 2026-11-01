// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelcontentscale.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QPushButton>
#include <QVBoxLayout>
namespace OpenGateTest {
template<class Check> void timelineContentScaleChecks(Check check) {
    QWidget timeline;
    timeline.setObjectName("frameConformTimelines");
    timeline.setAttribute(Qt::WA_DontShowOnScreen);
    auto layout = new QVBoxLayout(&timeline);
    QPushButton toolbar("Native tools"); toolbar.setFixedHeight(31); layout->addWidget(&toolbar);
    QGraphicsScene scene;
    auto item = scene.addRect(QRectF(-100, -50, 200, 100));
    auto view = new QGraphicsView(&scene, &timeline);
    view->setObjectName("m_TiTimelineView"); layout->addWidget(view);
    timeline.resize(700, 400); timeline.show(); QApplication::processEvents();
    const auto toolbarFont = toolbar.font(), baselineFont = view->font();
    const auto baseline = view->transform();
    const auto rect = item->rect();
    Unbound::PanelContentScale scale;
    for (double factor : {1., 2., .8, 1.5, 3.}) {
        scale.setValue(factor);
        check(scale.apply(&timeline), "Timeline content uses the coherent viewport on ordinary launch");
        for (int i = 0; i < 6; ++i) QApplication::processEvents();
        check(view->scene() == &scene && view->transform() == baseline && item->rect() == rect && view->font() == baselineFont,
              "content size never changes the native scene transform, item geometry or fonts");
        check(toolbar.height() == 31 && toolbar.font() == toolbarFont, "Timeline toolbar stays compact");
    }
    QGraphicsScene stackedScene;
    auto stacked = new QGraphicsView(&stackedScene, &timeline);
    stacked->setObjectName("m_TiTimelineView"); layout->addWidget(stacked); stacked->show();
    for (int i = 0; i < 12; ++i) QApplication::processEvents();
    check(timeline.findChildren<QWidget *>("openGateTimelineContentViewport").size() == 2,
          "newly created stacked view receives the same coherent content presentation");
    check(scale.suspend() && view->parentWidget() == &timeline && view->transform() == baseline && scale.value() == 3.,
          "native return restores source ownership while retaining content preference");
    scale.setValue(1.);
    check(scale.apply(&timeline) && !timeline.findChildren<QWidget *>("openGateTimelineContentViewport").isEmpty(),
          "reset to 100 percent retains draggable header controls");
    check(scale.suspend(), "final native restoration succeeds");
}
}
