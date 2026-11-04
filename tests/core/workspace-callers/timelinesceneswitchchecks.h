// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinecontentviewport.h"

namespace OpenGateTest {
// Presentation cache coverage only; fixture scenes are not native timeline IDs.
template <class Check> void timelineSceneSwitchChecks(Check check)
{
    TimelineViewportFixture fixture;
    auto tracks = qobject_cast<QGraphicsView *>(fixture.view->tracksProxy->widget());
    QGraphicsScene first, second;
    for (auto scene : {&first, &second}) scene->setSceneRect(0, 0, 4000, 400);
    first.addRect(0, 0, 4000, 400, QPen(Qt::NoPen), QBrush(Qt::red));
    second.addRect(0, 0, 4000, 400, QPen(Qt::NoPen), QBrush(Qt::green));
    Unbound::TimelineContentViewport viewport(fixture.view, &fixture.timeline);
    viewport.setGeometry(0, 31, 900, 500);
    viewport.show();
    auto pixel = [&] {
        const auto image = viewport.presentation()->grab().toImage();
        const auto point = QPointF((viewport.headerWidth()+40)*1.47, 65*1.47) * image.devicePixelRatioF();
        return image.pixelColor(point.toPoint());
    };
    viewport.configure(1.47, 160, 0, true);
    tracks->setScene(&first);
    TimelineViewportFixture::settle();
    check(pixel() == QColor(Qt::red), "first nested scene appears in the original native view");
    auto root = fixture.view->scene();
    for (int i = 0; i < 8; ++i) {
        auto scene = i%2 ? &first : &second;
        tracks->setScene(scene);
        TimelineViewportFixture::settle();
        check(fixture.view->scene() == root && tracks->scene() == scene,
              "switch reuses the root and nested view while native scene identity changes");
        check(pixel() == QColor(i%2 ? Qt::red : Qt::green),
              "scene switch replaces cached pixels without changing scale or resizing");
    }
    // A later edit in the newly selected scene must remain subscribed too.
    auto fill = second.addRect(0, 0, 4000, 400, QPen(Qt::NoPen), QBrush(Qt::cyan));
    tracks->setScene(&second);
    TimelineViewportFixture::settle();
    check(pixel() == QColor(Qt::cyan), "new scene edits reach the current presentation");
    fill->setBrush(Qt::yellow);
    TimelineViewportFixture::settle();
    check(pixel() == QColor(Qt::yellow), "new scene keeps its damage subscription after switching");
}
}
