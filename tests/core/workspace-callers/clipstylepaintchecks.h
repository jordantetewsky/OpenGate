// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/timelineclipcolorpreview.h"
#include "timelineviewportfixtures.h"

// Deliberately not a QObject: native semantic label discovery must support RTTI
// without requiring a replacement native item or retained child pointer.
class UiTiGraphicsLabelItem : public QGraphicsItem {
  public:
    QPixmap image;
    explicit UiTiGraphicsLabelItem(QGraphicsItem *parent) : QGraphicsItem(parent) {
        image = QPixmap(130, 14);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setPen(Qt::white);
        QFont font;
        font.setPixelSize(11);
        p.setFont(font);
        p.drawText(QPointF(3, 11), "Cached native name");
    }
    QRectF boundingRect() const override {
        return {0, 0, 130, 14};
    }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override {
        p->drawPixmap(QPointF(), image);
    }
};

namespace OpenGateTest {
template <class Check> void clipStylePaintChecks(Check check) {
    {
        TimelineViewportFixture fixture;
        fixture.view->clip->hide();
        QVector<UiTiClipContainer *> clips;
        for (int i = 0; i < 32; ++i) {
            auto clip = new UiTiClipContainer;
            clip->setFlag(QGraphicsItem::ItemHasNoContents);
            clip->setPos(i * 250, 0);
            fixture.view->tracksScene->addItem(clip);
            clip->setSelected(true);
            clips.append(clip);
        }
        Unbound::TimelineClipColorPreview preview(fixture.view, fixture.view);
        auto bindings = Unbound::TimelineClipColorBindings::find(fixture.view);
        const auto before = bindings->revision();
        Unbound::ColorPreset preset;
        preset.id = "bulk";
        preset.name = "Bulk";
        check(preview.applySelected(preset) == 32 && bindings->revision() == before + 1,
              "bulk selection updates every binding with one coalesced display revision");
        for (auto clip : clips)
            delete clip;
        check(bindings->regions().isEmpty(),
              "deleted native clip objects release their weak bindings without stale paint");
    }
    for (bool skin : {false, true})
        for (double factor : {.7, 1., 1.47, 2.13, 3.}) {
            TimelineViewportFixture fixture;
            fixture.view->clip->hide();
            auto tracks = dynamic_cast<BatchedClipSceneView *>(fixture.view->tracksProxy->widget());
            tracks->clipBackground = true;
            tracks->clipSkins = skin;
            QImage source(244, 164, QImage::Format_ARGB32);
            source.fill(QColor(67, 118, 161));
            source.setPixelColor(0, 0, QColor(0, 0, 0, 100));
            tracks->skin = QPixmap::fromImage(source);
            tracks->skin.setDevicePixelRatio(2);
            auto clip = new UiTiClipContainer;
            clip->setFlag(QGraphicsItem::ItemHasNoContents);
            clip->setSelected(true);
            fixture.view->tracksScene->addItem(clip);
            auto details = new UiTiClipContainer;
            details->paintBody = false;
            details->setParentItem(clip);
            details->setFlag(QGraphicsItem::ItemIsSelectable, false);
            auto label = new UiTiGraphicsLabelItem(clip);
            label->setPos(60, 78);
            label->setZValue(3);
            auto other = new UiTiClipContainer;
            other->setFlag(QGraphicsItem::ItemHasNoContents);
            other->setPos(0, 110);
            fixture.view->tracksScene->addItem(other);
            auto playhead = fixture.view->tracksScene->addRect(QRectF(218, 0, 2, 205), Qt::NoPen,
                                                               QColor("#ff392c"));
            playhead->setZValue(999);
            Unbound::PanelContentScale scale;
            scale.restoreState({{"interface_scale", factor},
                                {"timeline_header_width", 160.},
                                {"timeline_header_scale", .7}});
            scale.apply(&fixture.timeline);
            TimelineViewportFixture::settle();
            auto content = dynamic_cast<Unbound::TimelineContentViewport *>(
                fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
            auto frame = [&] {
                tracks->viewport()->update();
                TimelineViewportFixture::settle();
                return content->presentation()->grab().toImage();
            };
            auto pixel = [&](const QImage &im, QPointF point) {
                return im.pixelColor((QPointF((content->headerWidth() + point.x()) * factor,
                                              (40 + point.y()) * factor) *
                                      im.devicePixelRatioF())
                                         .toPoint());
            };
            const auto before = frame();
            Unbound::TimelineClipColorPreview preview(fixture.view, content);
            Unbound::ColorPreset style;
            style.id = "vfx-trial";
            style.name = "VFX trial";
            style.color = QColor("#ff3c0a");
            style.stops = {{"left", style.color, 0, 1}, {"right", QColor("#0a50f0"), 1, 1}};
            check(preview.applySelected(style) == 1,
                  "opaque multi-stop preset attaches to the selected native clip");
            frame();
            const auto gradient = frame();
            const auto left = pixel(gradient, {60, 8}), right = pixel(gradient, {200, 20});
            if (!(left.red() > right.red() + 70 && right.blue() > left.blue() + 70))
                std::fprintf(stderr, "style skin=%d scale=%.2f left=%s right=%s\n", skin, factor,
                             qPrintable(left.name()), qPrintable(right.name()));
            check(left.red() > right.red() + 70 && right.blue() > left.blue() + 70,
                  "gradient spans the native clip body at every scale and skin path");
            check(preview.selectedPresetId() == style.id,
                  "selected preset identity is retained separately from native swatch state");
            check(pixel(gradient, {200, 130}) == pixel(before, {200, 130}),
                  "gradient preserves a neighbor sharing the same native skin");
            check(pixel(gradient, {20, 15}) == pixel(before, {20, 15}) &&
                      pixel(gradient, {20, 45}) == pixel(before, {20, 45}),
                  "gradient preserves native thumbnail and waveform pixels");
            check(pixel(gradient, {219, 20}) == pixel(before, {219, 20}),
                  "playhead rectangle remains native above gradient paint");
            check(Unbound::colorContrast(pixel(gradient, {61, 79}), Qt::white) >= 4.5,
                  "semantic cached label receives a readable backing");
            // Repeat frames must reuse deterministic cached patterns, including the
            // twelve distinct texture variants; there is no animated grain.
            for (const auto &texture : Unbound::ColorPreset::textures()) {
                style.texture = texture;
                style.strength = 1;
                preview.applySelected(style);
                frame();
                const auto textured = frame();
                check(frame() == textured, "repeated preset frames are pixel-stable");
                if (texture != "flat") {
                    bool different = false;
                    for (int x = 180; x < 212; ++x)
                        for (int y = 10; y < 29; ++y)
                            different |= pixel(textured, {double(x), double(y)}) !=
                                         pixel(gradient, {double(x), double(y)});
                    check(different,
                          "each texture family changes only native clip background paint");
                }
                check(pixel(textured, {20, 15}) == pixel(before, {20, 15}) &&
                          pixel(textured, {20, 45}) == pixel(before, {20, 45}) &&
                          pixel(textured, {219, 20}) == pixel(before, {219, 20}),
                      "texture leaves native foreground and playhead unchanged");
            }
            const auto texturedFrame = frame();
            const auto appearance = Unbound::TimelineClipColorBindings::find(fixture.view)
                                        ->regions()
                                        .front()
                                        .style->json();
            auto display = Unbound::ColorDisplaySettings::shared();
            display->setTexturesEnabled(false);
            TimelineViewportFixture::settle();
            check(content->presentation()->grab().toImage() == gradient,
                  "texture-off immediately paints the base gradient without native input or edit");
            check(Unbound::TimelineClipColorBindings::find(fixture.view)
                          ->regions()
                          .front()
                          .style->json() == appearance,
                  "texture-off preserves exact preset and assignment parameters");
            display->setTexturesEnabled(true);
            TimelineViewportFixture::settle();
            check(content->presentation()->grab().toImage() == texturedFrame,
                  "texture-on immediately restores identical cached texture pixels");
            style.opacity = .5;
            check(preview.applySelected(style) == 0,
                  "unverified native multi-layer alpha is rejected without replacing the existing "
                  "preset");
            style.opacity = 1;
            style.texture = "flat";
            style.stops.clear();
            style.color = QColor("#7b33ac");
            preview.applySelected(style);
            frame();
            check(pixel(frame(), {200, 20}) == style.color,
                  "gradient-to-solid preset change replaces all body paint");
            preview.applySelected(Qt::red);
            frame();
            check(
                pixel(frame(), {200, 20}) == QColor(Qt::red) &&
                    preview.selectedPresetId().isEmpty(),
                "RGB command replaces the preset binding rather than leaving a competing override");
            preview.clear();
            check(frame() == before,
                  "clear restores exact native pixels after textures and cached labels");
        }
}
} // namespace OpenGateTest
