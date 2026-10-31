// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/timelinerenderprobe.h"
#include <QGraphicsItem>

namespace OpenGateTest {
// Models the observed coordinate contract, not Resolve's implementation. Native
// acceptance still requires the opt-in probe images from the actual scene.
class FixedGlyphItem : public QGraphicsItem {
public:
    QRectF boundingRect() const override { return {0, 0, 180, 60}; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        painter->fillRect(QRectF(0, 0, 180, 60), Qt::black);
        const auto first = painter->worldTransform().map(QPointF(10, 20));
        const auto second = painter->worldTransform().map(QPointF(30, 20));
        painter->save();
        painter->setWorldTransform(QTransform());
        QFont font;
        font.setPixelSize(16);
        painter->setFont(font);
        painter->setPen(Qt::red);
        painter->drawText(first, "H");
        painter->drawText(second, "H");
        painter->restore();
    }
};

inline int redInkHeight(const QImage &image) {
    int first = image.height(), last = -1;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x) {
            const auto color = image.pixelColor(x, y);
            if (color.red() > 180 && color.green() < 60 && color.blue() < 60) {
                first = qMin(first, y);
                last = qMax(last, y);
            }
        }
    return last >= first ? last - first + 1 : 0;
}

template<class Check> void timelineRenderProbeChecks(Check check) {
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 180, 60);
    auto item = new FixedGlyphItem;
    scene.addItem(item);
    QGraphicsView original(&scene);
    const auto views = scene.views();
    const auto rect = scene.sceneRect();
    const auto transform = original.transform();
    const auto baseline = Unbound::TimelineRenderProbe::render(&scene, rect, 1.);
    const auto coherent = Unbound::TimelineRenderProbe::render(&scene, rect, 2.);

    QImage stretched(180, 60, QImage::Format_ARGB32_Premultiplied);
    stretched.fill(Qt::transparent);
    {
        QPainter painter(&stretched);
        scene.render(&painter, rect, QRectF(0, 0, 90, 30), Qt::IgnoreAspectRatio);
    }
    const auto baselineHeight = redInkHeight(baseline);
    check(baselineHeight > 0 && redInkHeight(stretched) <= baselineHeight + 2,
        "coordinate-only transform reproduces fixed glyph size despite expanded positions");
    check(redInkHeight(coherent) >= baselineHeight * 1.8 && redInkHeight(coherent) <= baselineHeight * 2.3,
        "backing-store rendering enlarges glyphs together with their positions");
    check(coherent.size() == baseline.size() && coherent.devicePixelRatio() == 2.,
        "content enlargement keeps the same physical viewport size");
    check(scene.views() == views && scene.sceneRect() == rect && original.transform() == transform && item->pos().isNull(),
        "render probe leaves native scene, view registration and geometry unchanged");
    check(Unbound::TimelineRenderProbe::render(&scene, rect, qQNaN()).isNull() &&
        Unbound::TimelineRenderProbe::render(&scene, QRectF(0, 0, 100000, 60), 2.).isNull(),
        "render probe rejects invalid factors and unbounded image allocation");

    QWidget timeline;
    timeline.setAttribute(Qt::WA_DontShowOnScreen);
    QGraphicsView candidate(&scene, &timeline);
    candidate.setObjectName("m_TiTimelineView");
    candidate.setGeometry(0, 0, 180, 60);
    timeline.resize(180, 60);
    timeline.show();
    candidate.show();
    QApplication::processEvents();
    const auto originalGeometry = candidate.geometry();
    const auto directory = Unbound::diagnosticDirectory() + "/TimelineRenderProbe/" + Unbound::diagnosticSessionId();
    qunsetenv("OPENGATE_TIMELINE_RENDER_PROBE");
    Unbound::TimelineRenderProbe probe;
    probe.capture(&timeline);
    check(!QDir(directory).exists(), "native comparison images require explicit probe opt-in");
    qputenv("OPENGATE_TIMELINE_RENDER_PROBE", "1");
    probe.capture(&timeline);
    probe.capture(&timeline);
    check(QDir(directory).entryList({"*.png"}, QDir::Files).size() == 2,
        "one native view produces one bounded comparison pair without repeated captures");
    check(candidate.geometry() == originalGeometry && candidate.transform().isIdentity() && candidate.parentWidget() == &timeline,
        "opt-in capture does not reparent or resize the live native view");
    qunsetenv("OPENGATE_TIMELINE_RENDER_PROBE");
}
}
