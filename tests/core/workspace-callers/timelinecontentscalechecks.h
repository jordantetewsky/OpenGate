// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelcontentscale.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QPushButton>
#include <QVBoxLayout>
namespace OpenGateTest {
template<class Check> void timelineContentScaleChecks(Check check) {
    QWidget timeline;timeline.setObjectName("frameConformTimelines");timeline.setAttribute(Qt::WA_DontShowOnScreen);
    auto layout=new QVBoxLayout(&timeline);QPushButton toolbar("Native tools");toolbar.setFixedHeight(31);layout->addWidget(&toolbar);
    QGraphicsScene scene;auto item=scene.addRect(QRectF(-100,-50,200,100));
    auto view=new QGraphicsView(&scene,&timeline);view->setObjectName("m_TiTimelineView");layout->addWidget(view);
    timeline.resize(700,400);timeline.show();QApplication::processEvents();
    const auto toolbarFont=toolbar.font();const auto baseline=view->transform();const auto rect=item->rect();
    Unbound::PanelContentScale scale;scale.setValue(2.0);
    qunsetenv("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT");
    check(!scale.apply(&timeline)&&view->transform()==baseline,
        "ordinary launch cannot enable the rejected Timeline transform experiment");
    qputenv("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT","0");
    check(!scale.apply(&timeline)&&toolbar.height()==31&&view->transform()==baseline,"unverified Timeline scaling cannot silently grow the toolbar");
    qputenv("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT","1");
    for(double factor:{2.,0.8,1.5,2.}) {
        scale.setValue(factor);check(scale.apply(&timeline),"opt-in native graphics scale applies");QApplication::processEvents();
        const auto hit=view->mapFromScene(QPointF(0,0));
        check(view->itemAt(hit)==item&&item->rect()==rect,"scaled scene preserves native item geometry and inverse pointer hits");
        check(toolbar.height()==31&&toolbar.font()==toolbarFont,"content scale preserves compact toolbar height/font");
    }
    view->viewport()->setAttribute(Qt::WA_PaintOnScreen);
    check(!Unbound::TimelineContentScale::supported(&timeline) && !scale.apply(&timeline),
        "native paint surfaces remain unavailable even with the experiment flag enabled");
    check(view->transform()==baseline && scale.value()==2. && toolbar.height()==31,
        "rejecting a native renderer restores prior transforms without changing preference or chrome");
    view->viewport()->setAttribute(Qt::WA_PaintOnScreen,false);
    check(scale.apply(&timeline),"ordinary Qt experiment can reattach after native renderer refusal");
    QGraphicsScene stackedScene;auto stacked=new QGraphicsView(&stackedScene,&timeline);
    stacked->setObjectName("m_TiTimelineView");layout->addWidget(stacked);stacked->show();
    for(int i=0;i<8;++i)QApplication::processEvents();
    check(qAbs(stacked->transform().m11()-2.)<0.001,"newly created stacked Timeline view inherits the retained content scale");
    scale.suspend();check(view->transform()==baseline&&scale.value()==2.,"transfer restores native transform while retaining scale preference");
    scale.apply(&timeline);scale.setValue(1.);scale.apply(&timeline);
    check(view->transform()==baseline,"reset restores exact native scene transform");
    qunsetenv("OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT");
}
}
