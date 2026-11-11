// SPDX-License-Identifier: MIT
#include "../../../src/core/clipcolorpreviewcommands.h"
#include "../../../src/core/panelcommandcatalog.h"
#include "../../core/colors/assignmentdocumentfixture.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include <QTemporaryDir>
#include <cstdio>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir files;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", files.path().toUtf8());
    qputenv("UNBOUND_SHORTCUTS_FILE", (files.path()+"/Shortcuts.json").toUtf8());
    qputenv("OPENGATE_COLOR_DISPLAY_FILE", (files.path()+"/Display.json").toUtf8());
    int failures = 0;
    const auto check = [&](bool good, const char *message) {
        if (!good) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    Unbound::ColorPalette palette(files.path()+"/Colors.json");
    auto a = *palette.find("red");
    a.name = "Sunset"; a.texture = "grain";
    a.stops = {{"left", Qt::red, 0, 1}, {"right", QColor("#ed3010"), 1, 1}};
    auto b = a; b.id = "forest"; b.name = "Second paint"; b.texture = "crosshatch";
    check(palette.update(a).isEmpty() && palette.update(b).isEmpty(), "prepare actual committed palette commands");
    check(Unbound::ClipColorFallback::nearest(a) == Unbound::ClipColorFallback::nearest(b),
          "custom-to-custom test has exactly the same native fallback");
    const auto path = files.path()+"/native-project.json";
    QByteArray savedAssignment;
    {
        OpenGateTest::AssignmentDocumentFixture document(path);
        QWidget main; main.setObjectName("UiMainWindow"); main.setAttribute(Qt::WA_DontShowOnScreen); main.resize(800, 600);
        QWidget page(&main); page.setObjectName("WStackPage_Conform"); page.resize(main.size()); page.show();
        QWidget panel(&page); panel.setObjectName("frameConformTimelines"); panel.resize(page.size()); panel.show();
        QGraphicsView view(&panel); view.setObjectName("m_TiTimelineView"); view.resize(panel.size());
        QGraphicsScene scene;
        auto clip = new UiTiClipContainer;
        clip->setProperty("fixtureNativeId", "timeline-A/clip-1"); scene.addItem(clip); clip->setSelected(true);
        view.setScene(&scene); view.show(); main.show();
        Unbound::PanelCommandCatalog commands(&main, [] { return true; });
        Unbound::ClipColorPreviewCommands colors(&main, &commands, {}, false, &palette, &document);
        auto settle = [&] { OpenGateTest::TimelineViewportFixture::settle(); };
        auto focus = [&] { QApplication::setActiveWindow(&main); view.setFocus(); settle(); };
        auto invoke = [&](const QString &id) { focus(); const bool accepted = commands.invoke("clip-color.timeline.preset."+id, "fixture"); settle(); return accepted; };
        check(invoke(a.id), "actual palette registry command executes");
        auto display = Unbound::TimelineClipColorBindings::find(&view);
        check(document.read(clip).present && display && display->presetId(clip) == a.id && document.history.count() == 1,
              "normal palette command creates one host event and materializes saved assignment");
        int trim = 0;
        class Trim : public QUndoCommand {
          public: explicit Trim(int &value) : value(value) {} void undo() override { value = 0; } void redo() override { value = 8; }
          private: int &value;
        };
        document.history.push(new Trim(trim));
        check(invoke(b.id) && document.history.count() == 3 && display->presetId(clip) == b.id,
              "same-fallback custom change is its own event after an ordinary host edit");
        document.history.undo(); settle();
        check(display->presetId(clip) == a.id && trim == 8, "Undo reaches custom A before touching preceding trim");
        document.history.undo(); settle();
        check(trim == 0 && display->presetId(clip) == a.id, "next Undo reaches the ordinary edit in the same host stack");
        document.history.undo(); settle();
        check(!document.read(clip).present && display->presetId(clip).isEmpty() && document.read(clip).nativeColor == "ClipColorBlue",
              "first-assignment Undo restores Blue and true absence together");
        document.history.redo(); document.history.redo(); document.history.redo(); settle();
        check(trim == 8 && display->presetId(clip) == b.id, "Redo traverses ordinary/custom events in original order");
        auto adapter = Unbound::TimelineClipColorPreview::ensure(&view, &palette, &document);
        document.rejectNext = true;
        const auto beforeReject = document.read(clip).bytes;
        check(adapter->applySelected(a) == 0 && document.read(clip).bytes == beforeReject && display->presetId(clip) == b.id,
              "host rejection never paints a temporary success over the saved assignment");

        QMenu menu; menu.setAttribute(Qt::WA_DontShowOnScreen); menu.setObjectName("markSetClipColor");
        auto purple = menu.addAction("Purple"); purple->setObjectName("markSetClipColorPurple");
        auto clear = menu.addAction("Clear Color"); clear->setObjectName("markSetClipColorDefault");
        int originalCalls = 0;
        QObject::connect(purple, &QAction::triggered, &main, [&] { ++originalCalls; });
        focus();
        QContextMenuEvent context(QContextMenuEvent::Mouse, QPoint(5, 5)); QApplication::sendEvent(&view, &context);
        menu.popup(QPoint(300, 300)); settle();
        QAction *stock = nullptr;
        for (auto action : menu.actions()) if (action->objectName() == "openGateNativeClipColor_Purple") stock = action;
        check(stock, "candidate native swatch row routes to the common assignment operation");
        const int count = document.history.count();
        if (stock) stock->trigger(); menu.hide(); settle();
        check(!document.read(clip).present && document.read(clip).nativeColor == "ClipColorPurple" &&
              originalCalls == 0 && document.history.count() == count+1,
              "stock selection replaces custom metadata/fallback in exactly one host event without replay");
        document.history.undo(); settle();
        check(display->presetId(clip) == b.id, "Undo native replacement restores the full custom paint");
        check(adapter->assignNative("") == 1 && !document.read(clip).present && document.read(clip).nativeColor.isEmpty(),
              "Clear removes explicit assignment and uses native inheritance rather than copying RGB");
        document.history.undo(); settle();
        check(display->presetId(clip) == b.id, "Undo Clear restores prior custom assignment");

        // Recreate the Timeline scene and graphics objects as native tabs do.
        QGraphicsScene other;
        auto otherClip = new UiTiClipContainer; otherClip->setProperty("fixtureNativeId", "timeline-B/clip-1");
        other.addItem(otherClip); otherClip->setSelected(true);
        view.hide(); view.setScene(&other); view.show(); settle();
        check(display->presetId(otherClip).isEmpty(), "timeline B never inherits A's graphics bindings");
        scene.removeItem(clip); delete clip;
        clip = new UiTiClipContainer; clip->setProperty("fixtureNativeId", "timeline-A/clip-1");
        scene.addItem(clip); clip->setSelected(true);
        view.hide(); view.setScene(&scene); view.show(); settle();
        check(display->presetId(clip) == b.id, "A to B to recreated A restores from native document identity");
        QGraphicsView secondView(&panel); secondView.resize(300, 200);
        QGraphicsScene secondScene;
        auto secondClip = new UiTiClipContainer; secondClip->setProperty("fixtureNativeId", "timeline-A/clip-2");
        secondScene.addItem(secondClip); secondClip->setSelected(true); secondView.setScene(&secondScene); secondView.show();
        const auto secondAdapter = Unbound::TimelineClipColorPreview::ensure(&secondView, &palette, &document);
        check(secondAdapter->applySelected(b) == 1, "a second registered view uses the same document assignments");
        settle();
        const auto beforeEdit = document.history.index();
        b.name = "Edited paint"; b.texture = "stipple";
        check(palette.update(b).isEmpty(), "edit assigned swatch while retaining stable ID"); settle();
        const auto updated = Unbound::ClipColorAssignment::decode(document.read(clip).bytes);
        check(updated && updated->paint == b && display->regions().front().style->texture == "stipple",
              "swatch edit updates loaded clip's persistent definition and displayed paint");
        const auto secondUpdated = Unbound::ClipColorAssignment::decode(document.read(secondClip).bytes);
        check(secondUpdated && secondUpdated->paint == b && document.history.index() == beforeEdit+1,
              "one swatch edit updates all registered views in one host event");
        auto next = palette.state();
        for (int i = next.presets.size()-1; i >= 0; --i) if (next.presets[i].id == b.id) next.presets.removeAt(i);
        const auto fallback = document.read(clip).nativeColor;
        check(palette.replace(next).isEmpty(), "delete assigned swatch"); settle();
        check(!document.read(clip).present && document.read(clip).nativeColor == fallback && scene.items().contains(clip),
              "deleting a swatch preserves clip and leaves nearest stock fallback");
        check(!document.read(secondClip).present && secondScene.items().contains(secondClip),
              "deletion also replaces the other registered view's assignment without deleting its clip");
        document.history.undo(); settle();
        check(document.read(clip).present && display->presetId(clip) == b.id,
              "host Undo restores deleted assignment with its embedded definition");
        savedAssignment = document.read(clip).bytes;
    }
    {
        OpenGateTest::AssignmentDocumentFixture reopened(path);
        QGraphicsView view; view.setAttribute(Qt::WA_DontShowOnScreen); view.resize(600, 400);
        QGraphicsScene scene; auto clip = new UiTiClipContainer;
        clip->setProperty("fixtureNativeId", "timeline-A/clip-1"); scene.addItem(clip); view.setScene(&scene); view.show();
        auto adapter = Unbound::TimelineClipColorPreview::ensure(&view, &palette, &reopened);
        OpenGateTest::TimelineViewportFixture::settle();
        check(reopened.read(clip).bytes == savedAssignment && Unbound::TimelineClipColorBindings::find(&view)->presetId(clip) == b.id,
              "fresh document/view restores exact paint without requiring personal preset presence");
        check(adapter->persistent(), "reopened display uses document-backed path");
    }
    {
        // Palette UI/model may be recreated while native document/views remain.
        // Reuse the address to catch a pointer-only subscription identity bug.
        OpenGateTest::AssignmentDocumentFixture document(files.path()+"/lifetime-project.json");
        QGraphicsView view; view.setAttribute(Qt::WA_DontShowOnScreen); view.resize(600, 400);
        QGraphicsScene scene; auto clip = new UiTiClipContainer;
        clip->setProperty("fixtureNativeId", "lifetime/clip"); scene.addItem(clip); clip->setSelected(true);
        view.setScene(&scene); view.show();
        alignas(Unbound::ColorPalette) unsigned char storage[sizeof(Unbound::ColorPalette)];
        auto first = new (storage) Unbound::ColorPalette(files.path()+"/lifetime-colors.json");
        auto adapter = Unbound::TimelineClipColorPreview::ensure(&view, first, &document);
        check(adapter->applySelected(*first->find("red")) == 1, "assign through first palette lifetime");
        first->~ColorPalette();
        auto second = new (storage) Unbound::ColorPalette(files.path()+"/lifetime-colors.json");
        Unbound::TimelineClipColorPreview::ensure(&view, second, &document);
        auto edited = *second->find("red"); edited.color = QColor("#229944");
        const int before = document.history.index();
        check(second->update(edited).isEmpty(), "recreated palette accepts an edit");
        OpenGateTest::TimelineViewportFixture::settle();
        const auto record = Unbound::ClipColorAssignment::decode(document.read(clip).bytes);
        check(record && record->paint.color == edited.color && document.history.index() == before+1,
              "recreated palette updates existing assignments once without a dangling subscription");
        second->~ColorPalette();
    }
    if (!failures) std::puts("PASS: real palette/menu commands, host history interleaving, recreated timelines, saved document reopen, edit/delete assignments");
    return failures ? 1 : 0;
}
