// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/clipcolorpreviewcommands.h"
#include "../workspace-callers/timelineviewportfixtures.h"
namespace OpenGateTest {
template <class Check>
void presetCommandChecks(Check check, QWidget *main, Unbound::PanelCommands &commands,
                         TimelineViewportFixture &timeline, UiTiClipContainer *clip,
                         const QString &directory) {
    Unbound::ColorPalette palette(directory + "/command-colors.json");
    const auto nativeBaseline = clip->fill;
    Unbound::ColorPreset preset;
    preset.id = "vfx-custom";
    preset.name = "My VFX";
    preset.category = "custom";
    preset.color = QColor("#ae227d");
    preset.stops = {{"a", preset.color, 0, 1}, {"b", QColor("#22a8de"), 1, 1}};
    preset.texture = "grain";
    check(palette.update(preset).isEmpty(),
          "palette commits an editable gradient before commands are wired");
    Unbound::ClipColorPreviewCommands colors(main, &commands, {}, false, &palette);
    const auto commandId = "clip-color.timeline.preset." + preset.id;
    auto count = [&] {
        int n = 0;
        for (const auto &entry : commands.entries())
            n += entry.id == commandId;
        return n;
    };
    check(count() == 1 && commands.commandScope(commandId) == "frameConformTimelines",
          "committed custom swatch has one stable Timeline-scoped operation");
    QMenu native;
    native.setAttribute(Qt::WA_DontShowOnScreen);
    native.setObjectName("markSetClipColor");
    auto clear = native.addAction("Clear Color");
    clear->setObjectName("markSetClipColorDefault");
    auto blue = native.addAction("Blue");
    blue->setObjectName("markSetClipColorBlue");
    blue->setCheckable(true);
    blue->setChecked(true);
    auto orange = native.addAction("Orange");
    orange->setObjectName("markSetClipColorOrange");
    int nativeCalls = 0;
    QObject::connect(&native, &QMenu::triggered, main, [&](QAction *) { ++nativeCalls; });
    auto show = [&] {
        QApplication::setActiveWindow(main);
        timeline.view->setFocus();
        TimelineViewportFixture::settle();
        QContextMenuEvent context(QContextMenuEvent::Mouse, QPoint(5, 5));
        QApplication::sendEvent(timeline.view, &context);
        native.popup(QPoint(300, 300));
        TimelineViewportFixture::settle();
    };
    auto row = [&](const QString &id) -> QAction * {
        for (auto action : native.actions())
            if (action->objectName() == "openGatePresetColor_" + id)
                return action;
        return nullptr;
    };
    show();
    auto action = row(preset.id);
    check(action && action->isEnabled() && action->text() == preset.name &&
              native.actions().contains(blue) && native.actions().contains(orange),
          "custom categories append choices without replacing native swatches");
    if (action)
        action->trigger();
    native.hide();
    TimelineViewportFixture::settle();
    auto bindings = Unbound::TimelineClipColorBindings::find(timeline.view);
    check(bindings && bindings->presetId(clip) == preset.id && nativeCalls == 0 &&
              blue->isChecked(),
          "custom menu operation reaches actual selected clip without triggering or modifying "
          "native swatches");
    show();
    check(row(preset.id) && row(preset.id)->isChecked(),
          "reopening the palette reflects the selected custom preset");
    native.hide();
    TimelineViewportFixture::settle();
    preset.name = "Renamed VFX";
    preset.color = QColor("#219c8a");
    preset.stops.clear();
    preset.texture = "flat";
    const auto beforeUpdate = bindings->revision();
    check(palette.update(preset).isEmpty() && count() == 1,
          "Update Preset replaces the existing command rather than appending duplicates");
    check(bindings->regions().front().style &&
              bindings->regions().front().style->color == preset.color &&
              bindings->revision() == beforeUpdate + 1,
          "Update Preset refreshes its existing clip assignment without manually reapplying it");
    TimelineViewportFixture::settle();
    auto content = dynamic_cast<Unbound::TimelineContentViewport *>(
        timeline.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    if (content) {
        const auto frame = content->presentation()->grab().toImage();
        const auto at = QPointF(content->headerWidth() + 190, 60) * frame.devicePixelRatioF();
        if (frame.pixelColor(at.toPoint()) != preset.color || clip->fill != nativeBaseline)
            std::fprintf(stderr,
                         "preset frame pixel=%s expected=%s native=%s baseline=%s at=%d,%d\n",
                         qPrintable(frame.pixelColor(at.toPoint()).name()),
                         qPrintable(preset.color.name()), qPrintable(clip->fill.name()),
                         qPrintable(nativeBaseline.name()), at.toPoint().x(), at.toPoint().y());
        check(
            frame.pixelColor(at.toPoint()) == preset.color && clip->fill == nativeBaseline,
            "saved preset update reaches displayed pixels while native clip color remains intact");
    } else
        check(false, "preset update fixture retains its real presentation viewport");
    QAction trackColor("Native track color", main);
    trackColor.setObjectName("timelineSetTrackColorGreen");
    main->addAction(&trackColor);
    QObject::connect(&trackColor, &QAction::triggered, main, [&] {
        clip->fill = QColor("#449366");
        clip->update();
    });
    TimelineViewportFixture::settle();
    trackColor.trigger();
    TimelineViewportFixture::settle();
    if (content) {
        const auto frame = content->presentation()->grab().toImage();
        const auto at = QPointF(content->headerWidth() + 190, 60) * frame.devicePixelRatioF();
        check(bindings->presetId(clip) == preset.id &&
                  frame.pixelColor(at.toPoint()) == preset.color,
              "underlying native track-color repaint preserves the explicit custom clip paint");
        clear->trigger();
        TimelineViewportFixture::settle();
        const auto inherited = content->presentation()->grab().toImage();
        check(bindings->presetId(clip).isEmpty() &&
                  inherited.pixelColor(at.toPoint()) == clip->fill,
              "native Clear releases the custom override to the current native paint");
        check(commands.invoke(commandId, "fixture"),
              "custom clip paint can be reassigned after Clear without changing its saved ID");
    }
    QString label;
    for (const auto &entry : commands.entries())
        if (entry.id == commandId)
            label = entry.label;
    check(label.endsWith(preset.name),
          "renaming refreshes the command's visible label while retaining its ID");
    show();
    check(row(preset.id) && row(preset.id)->text() == preset.name,
          "committed preset edits refresh the same menu from the shared palette");
    native.hide();
    TimelineViewportFixture::settle();
    check(commands.invoke(commandId, "fixture") && bindings->regions().front().style &&
              bindings->regions().front().style->color == preset.color,
          "mapped operation reads the latest committed definition");
    auto other = new UiTiClipContainer;
    other->setPos(0, 110);
    timeline.view->tracksScene->addItem(other);
    auto surface = qobject_cast<QGraphicsView *>(timeline.view->tracksProxy->widget());
    check(bindings->apply(surface, other, preset),
          "another clip can retain an independent preset binding");
    const auto beforeBulkUpdate = bindings->revision();
    preset.color = QColor("#c63a12");
    check(palette.update(preset).isEmpty(),
          "a shared preset can be edited after multiple assignments");
    bool matching = true;
    for (const auto &region : bindings->regions())
        matching &= region.style && region.style->color == preset.color;
    check(bindings->regions().size() == 2 && matching &&
              bindings->revision() == beforeBulkUpdate + 1,
          "one saved swatch edit refreshes all its assigned clips with one display revision");
    auto different = preset;
    different.id = "unrelated-swatch";
    different.name = "Other swatch";
    const auto beforeUnrelated = bindings->revision();
    check(palette.update(different).isEmpty() && bindings->revision() == beforeUnrelated,
          "unrelated saved colors do not invalidate existing clip paint");
    QAction unrelated("Other command", main);
    unrelated.setObjectName("timelineOtherCommand");
    main->addAction(&unrelated);
    QAction nativeColor("Native Orange", main);
    nativeColor.setObjectName("markSetClipColorOrange");
    nativeColor.setShortcut(QKeySequence(Qt::Key_F9));
    main->addAction(&nativeColor);
    int nativeOperations = 0;
    QObject::connect(&nativeColor, &QAction::triggered, main, [&] {
        ++nativeOperations;
        clip->fill = QColor("#eb6e01");
        clip->update();
    });
    TimelineViewportFixture::settle();
    unrelated.trigger();
    check(bindings->presetId(clip) == preset.id, "unrelated native commands preserve custom paint");
    QLineEdit rename(timeline.view);
    rename.show();
    rename.setFocus();
    TimelineViewportFixture::settle();
    nativeColor.trigger();
    check(bindings->presetId(clip) == preset.id,
          "native color observation does not change bindings while a text field is active");
    timeline.view->setFocus();
    TimelineViewportFixture::settle();
    const auto operationsBefore = nativeOperations;
    QShortcutEvent nativeShortcut(QKeySequence(Qt::Key_F9), 0, false);
    QApplication::sendEvent(&nativeColor, &nativeShortcut);
    TimelineViewportFixture::settle();
    check(nativeOperations == operationsBefore + 1 && bindings->presetId(clip).isEmpty() &&
              bindings->presetId(other) == preset.id,
          "native QAction shortcut executes once and replaces only selected clip paint");
    preset.color = QColor("#4e288c");
    palette.update(preset);
    check(bindings->presetId(clip).isEmpty(),
          "later palette updates cannot revive a cleared clip assignment");
    delete other;
    auto next = palette.state();
    for (int i = next.presets.size() - 1; i >= 0; --i)
        if (next.presets[i].id == preset.id)
            next.presets.removeAt(i);
    check(palette.replace(next).isEmpty() && count() == 0 && !commands.invoke(commandId, "fixture"),
          "removing a custom preset retires its command without retargeting its stable ID");
    show();
    check(!row(preset.id) && native.actions().contains(blue) && native.actions().contains(orange) &&
              blue->text() == "Blue" && orange->text() == "Orange",
          "removing a custom preset preserves original native actions and labels");
    native.hide();
    TimelineViewportFixture::settle();
    bindings->clear();
}
} // namespace OpenGateTest
