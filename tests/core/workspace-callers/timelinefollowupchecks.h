// SPDX-License-Identifier: MIT
#pragma once
#include <QLineEdit>
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"

namespace OpenGateTest {
template<class Check> void timelineFollowupChecks(Check check) {
    QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1200,800);main.show();
    TimelineViewportFixture fixture(&main);QLineEdit source(&main);source.setGeometry(950,0,200,30);source.show();
    Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",3.},{"timeline_header_width",160.},{"timeline_header_scale",1.}});
    scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
    auto bar=fixture.timeline.findChild<QScrollBar *>("openGateTimelineHeaderScroll");
    QApplication::setActiveWindow(&main);source.setFocus();TimelineViewportFixture::settle();
    if(bar)TimelineViewportFixture::click(bar,QPointF(bar->width()-5,bar->height()/2));
    check(bar && QApplication::focusWidget()!=&source && Unbound::panelContains(&fixture.timeline,QApplication::focusWidget()),
          "clicking the pinned header scrollbar focuses Timeline instead of retaining Source/text focus");
    const int before=bar?bar->value():0;
    if(bar) {
        QWheelEvent wheel(QPointF(20,8),bar->mapToGlobal(QPoint(20,8)),QPoint(),QPoint(120,0),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(bar,&wheel);TimelineViewportFixture::settle();
    }
    check(bar && bar->value()!=before,"focused header scrollbar responds immediately to horizontal wheel input");
    scale.suspend();

    QGraphicsView native;native.setAttribute(Qt::WA_DontShowOnScreen);native.resize(300,200);native.show();
    Unbound::TimelineSceneUpdates updates(&native,&native);TimelineViewportFixture::settle();
    QStringList order;updates.layoutChanged=[&]{order.append("layout");};updates.paintReady=[&](const QRegion &){order.append("paint");};
    updates.schedulePaint(QRegion(native.viewport()->rect()));updates.geometryInvalidated();TimelineViewportFixture::settle();
    check(order.contains("layout") && order.contains("paint") && order.indexOf("layout")<order.indexOf("paint"),
          "a queued layout settles before an earlier queued presentation paint, avoiding an intermediate geometry frame");
}
}
