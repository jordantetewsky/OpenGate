// SPDX-License-Identifier: MIT
#include "../../../src/core/clipcolorpreviewcommands.h"
#include "../../../src/core/colormanagerpanel.h"
#include "../../../src/core/panelsession.h"
#include "../../../src/core/workspacearrangements.h"
#include "../../core/colors/perceptualchecks.h"
#include "../../core/colors/stockpalettechecks.h"
#include "../../core/colors/colordisplaychecks.h"
#include "../../core/colors/paintcachechecks.h"
#include "../../core/colors/palettetransferchecks.h"
#include "../../core/colors/presetupdatevisibilitychecks.h"
#include "../../core/colors/primitiveevidencechecks.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <cstdio>
class ColorMenuOrigin : public QWidget {
  public:
    using QWidget::QWidget;

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        event->accept();
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir directory;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", directory.path().toUtf8());
    qputenv("UNBOUND_TEST_HIDDEN_HOSTS", "1");
    qputenv("UNBOUND_SHORTCUTS_FILE", (directory.path() + "/Shortcuts.json").toUtf8());
    qputenv("OPENGATE_COLOR_DISPLAY_FILE", (directory.path()+"/ColorDisplay.json").toUtf8());
    int failures = 0;
    const auto check = [&](bool good, const char *message) {
        if (!good) {
            ++failures;
            std::fprintf(stderr, "FAIL: %s\n", message);
        }
    };
    OpenGateTest::perceptualColorChecks(check);
    OpenGateTest::stockPaletteChecks(check,directory.path());
    OpenGateTest::colorDisplayChecks(check,directory.path());
    OpenGateTest::paintCacheChecks(check);
    OpenGateTest::paletteTransferChecks(check,directory.path());
    OpenGateTest::presetUpdateVisibilityChecks(check,directory.path());
    OpenGateTest::primitiveEvidenceChecks(check);
    Unbound::ColorPreset p;
    p.id = "custom-test";
    p.name = "Readable VFX";
    p.category = "custom";
    p.stops = {{"a", Qt::white, 0, 1}, {"b", Qt::black, .5, 1}, {"c", Qt::yellow, 1, 1}};
    QString problem;
    Unbound::ColorPreset restored;
    check(Unbound::ColorPreset::parse(p.json(), restored, problem) && p == restored,
          "preset round trip preserves paint definition");
    auto invalid = p.json();
    invalid["opacity"] = 101;
    check(!Unbound::ColorPreset::parse(invalid, restored, problem),
          "invalid paint is rejected before mutation");
    Unbound::ColorPresetPaint paint;
    QSet<QByteArray> patterns;
    Unbound::ColorPaintTexture textures;
    for (const auto &kind : Unbound::ColorPreset::textures())
        if (kind != "flat") {
            p.texture = kind;
            const auto first = textures.tile(kind, .8), second = textures.tile(kind, .8);
            check(first.cacheKey() == second.cacheKey(),
                  "texture pixels are reused from bounded cache");
            QByteArray pixels(reinterpret_cast<const char *>(first.constBits()),
                              int(first.sizeInBytes()));
            patterns.insert(pixels);
            const auto label = paint.label(p);
            check(label.scrim.isValid() && label.contrast >= 4.5,
                  "textured gradients use contrast-safe label backing");
        }
    check(patterns.size() == 12, "all texture variants paint distinct static patterns");
    p.texture = "flat";
    check(paint.label(p).contrast >= 4.5, "mixed bright/dark gradient has safe text");
    p.stops.clear();
    p.color = Qt::yellow;
    check(paint.label(p).text == QColor(Qt::black), "bright solid chooses dark text");
    p.color = QColor(10, 20, 30);
    check(paint.label(p).text == QColor(Qt::white), "dark solid chooses light text");
    const auto path = directory.path() + "/Colors.json";
    Unbound::ColorPalette palette(path);
    check(palette.update(p).isEmpty(), "palette writes validated preset atomically");
    Unbound::ColorPalette reopened(path);
    check(reopened.find(p.id) && *reopened.find(p.id) == p, "personal palette survives reopen");
    const auto previous = Unbound::ColorPalette::encode(reopened.state());
    check(!reopened.import("{bad json").isEmpty() &&
              previous == Unbound::ColorPalette::encode(reopened.state()),
          "bad import preserves existing palette");
    Unbound::ColorPresetEditor editor;
    {
        auto gradient = p;
        gradient.stops = {
            {"left", Qt::red, 0, 1}, {"middle", Qt::green, .5, 1}, {"right", Qt::blue, 1, 1}};
        editor.setPreset(gradient);
        auto bar = dynamic_cast<Unbound::ColorGradientStopBar *>(
            editor.findChild<QWidget *>("openGateGradientStopBar"));
        bar->resize(400, 64);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(200, 45), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(bar, &press);
        QMouseEvent move(QEvent::MouseMove, QPointF(313, 45), Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
        QApplication::sendEvent(bar, &move);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(313, 45), Qt::LeftButton,
                            Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(bar, &release);
        const auto result = editor.value();
        check(result.stops[1].id == "middle" && std::abs(result.stops[1].position - .8) < .01 &&
                  result.stops[0].color == QColor(Qt::red) &&
                  result.stops[2].color == QColor(Qt::blue),
              "gradient handle drag changes identified stop without altering endpoints");
    }
    editor.setPreset(p);
    auto combo = editor.findChild<QComboBox *>("openGatePresetTexture");
    combo->setCurrentIndex(combo->findData("crosshatch-coarse"));
    check(editor.modified() && palette.find(p.id)->texture == "flat",
          "editing texture leaves committed palette unchanged");
    auto update = editor.findChild<QPushButton *>("openGateUpdatePreset");
    int committed = 0;
    editor.commit = [&](const auto &value) {
        ++committed;
        check(palette.update(value).isEmpty(), "Update routes to the one palette owner");
    };
    update->click();
    check(committed == 1 && palette.find(p.id)->texture == "crosshatch-coarse",
          "Update commits only the edited preset");
    {
        const auto draftPath = directory.path() + "/Drafts.json";
        auto changed = p;
        changed.texture = "stipple-coarse";
        {
            Unbound::ColorPaletteDrafts drafts(nullptr, draftPath);
            drafts.record(changed, p);
        }
        {
            Unbound::ColorPaletteDrafts drafts(nullptr, draftPath);
            check(drafts.get(p) == changed,
                  "uncommitted draft survives editor restart separately from saved palette");
            drafts.discard(p.id);
        }
        Unbound::ColorPaletteDrafts drafts(nullptr, draftPath);
        check(drafts.get(p) == p,
              "reverting discards recovered draft without changing saved definition");
    }
    QWidget main;
    main.setAttribute(Qt::WA_DontShowOnScreen);
    main.resize(1200, 900);
    QWidget page(&main);
    page.setObjectName("WStackPage_Conform");
    page.resize(main.size());
    page.show();
    main.show();
    qputenv("OPENGATE_COLOR_PALETTE_FILE", path.toUtf8());
    Unbound::ColorManagerPanel panel;
    const auto sharedPalette = &panel.palette();
    auto widget = panel.ensure(&page);
    check(widget == panel.ensure(&page), "entry points share one color manager widget");
    check(sharedPalette == &panel.palette(),
          "opening manager retains the palette already available to other surfaces");
    Unbound::ManagedPanelSession session;
    const auto error = session.undock(widget, &page, Unbound::ColorManagerPanel::label(), true);
    check(error.isEmpty() && session.active(),
          "owned manager uses the existing managed floating host");
    {
        Unbound::PanelCommands commands(&main);
        commands.context = [] { return true; };
        int invoked = 0;
        commands.add({"fixture.mixer", "Fixture mixer", [] { return true; }, [&] { ++invoked; }});
        commands.defaultBinding("fixture.mixer", "Shift+M");
        auto line = widget->findChild<QLineEdit *>("openGatePresetName");
        QApplication::setActiveWindow(session.window());
        line->setFocus();
        QApplication::processEvents();
        QKeyEvent over(QEvent::ShortcutOverride, Qt::Key_M, Qt::ShiftModifier, "M"),
            key(QEvent::KeyPress, Qt::Key_M, Qt::ShiftModifier, "M");
        QApplication::sendEvent(line, &over);
        QApplication::sendEvent(line, &key);
        check(invoked == 0 && line->text().endsWith("M"),
              "typing in manager fields retains native text editing instead of executing panel "
              "commands");
        int explicitClicks=0;commands.add({"fixture.texture-display","Fixture textures",[]{return true;},[&]{++explicitClicks;}});
        const auto text=line->text();
        check(commands.invoke("fixture.texture-display","opengate-command-toolbar")&&explicitClicks==1&&line->hasFocus()&&line->text()==text,
              "explicit toolbar operation preserves active text editor and caret without enabling typed commands");
    }
    check(session.dock().isEmpty() && !session.active() && !widget->isVisible(),
          "native return parks the manager without leaving a native slot footprint");
    {
        QMenuBar bar(&main);
        QMenu workspace("Workspace", &bar);
        workspace.setObjectName("workspace");
        bar.addMenu(&workspace);
        Unbound::WorkspaceArrangements menus(&main);
        menus.entries = [] { return QJsonArray{}; };
        menus.selected = [] { return QString(); };
        menus.invoke = [](QString, QString) {};
        menus.nativeWorkspace = [](QString) {};
        int opens = 0;
        menus.showColorManager = [&] { ++opens; };
        menus.wire();
        menus.wire();
        auto root = bar.findChild<QMenu *>("unboundWorkspaceUI");
        auto action = root ? root->findChild<QAction *>("openGateShowColorManager") : nullptr;
        auto native = root ? root->findChild<QAction *>("unboundRestoreNativeWorkspace") : nullptr;
        check(action && native &&
                  root->actions().indexOf(action) + 1 == root->actions().indexOf(native),
              "manager entry sits immediately above Original Resolve UI");
        if (action)
            action->trigger();
        QApplication::processEvents();
        check(opens == 1, "menu opens manager once through its existing owner");
        QWidget timeline(&page);
        timeline.setObjectName("frameConformTimelines");
        ColorMenuOrigin origin(&timeline);
        origin.setFocusPolicy(Qt::StrongFocus);
        timeline.show();
        origin.show();
        origin.setFocus();
        QContextMenuEvent context(QContextMenuEvent::Mouse, QPoint(1, 1));
        Unbound::PanelCommands commands(&main);
        Unbound::ClipColorPreviewCommands entries(&main, &commands, [&] { ++opens; }, false);
        QApplication::sendEvent(&origin, &context);
        QMenu colors;
        colors.setAttribute(Qt::WA_DontShowOnScreen);
        colors.setObjectName("markSetClipColor");
        colors.addAction("Orange");
        colors.popup(QPoint(10, 10));
        QApplication::processEvents();
        QAction *manager = nullptr;
        for (auto action : colors.actions())
            if (action->objectName() == "openGateClipColorManager")
                manager = action;
        int nativeCalls = 0;
        QObject::connect(&colors, &QMenu::triggered, [&](QAction *) { ++nativeCalls; });
        check(manager && colors.actions().front() == manager,
              "clip color submenu pins the same manager entry above native swatches");
        if (manager)
            manager->trigger();
        colors.hide();
        QElapsedTimer wait;
        wait.start();
        while (wait.elapsed() < 30)
            QApplication::processEvents();
        check(opens == 2 && nativeCalls == 0,
              "opening manager cannot invoke native clip-color handlers");
    }
    if (const auto snapshot = qEnvironmentVariable("OPENGATE_COLOR_MANAGER_SNAPSHOT");
        !snapshot.isEmpty()) {
        Unbound::ColorPreset demo = p;
        demo.stops = {{"first", QColor("#8729b4"), 0, 1},
                      {"middle", QColor("#00a6bd"), .45, 1},
                      {"end", QColor("#e6b123"), 1, 1}};
        demo.texture = "grain-medium";
        palette.update(demo);
        Unbound::ColorPalette previewPalette(path);
        Unbound::ColorPaletteDrafts previewDrafts(nullptr, path + ".drafts");
        Unbound::ColorManagerWidget view(previewPalette, previewDrafts);
        QPalette dark;
        dark.setColor(QPalette::Window, QColor(39, 39, 45));
        dark.setColor(QPalette::WindowText, QColor(220, 220, 225));
        dark.setColor(QPalette::Base, QColor(27, 27, 31));
        dark.setColor(QPalette::Text, QColor(220, 220, 225));
        dark.setColor(QPalette::Button, QColor(52, 52, 59));
        dark.setColor(QPalette::ButtonText, QColor(220, 220, 225));
        view.setPalette(dark);
        view.setAutoFillBackground(true);
        view.setAttribute(Qt::WA_DontShowOnScreen);
        view.resize(1100, 900);
        view.show();
        auto tree = view.findChild<QTreeWidget *>("openGatePaletteCategories");
        QTreeWidgetItemIterator item(tree);
        for (; *item; ++item)
            if ((*item)->data(0, Qt::UserRole).toString() == demo.id) {
                tree->setCurrentItem(*item);
                break;
            }
        QApplication::processEvents();
        check(view.grab().save(snapshot), "native manager screenshot writes successfully");
        view.resize(360, 1000);
        QApplication::processEvents();
        view.resize(360, 1000);
        QApplication::processEvents();
        if (view.width() > 360) {
            std::fprintf(stderr, "NARROW actual=%d minimum=%d hint=%d\n", view.width(),
                         view.minimumWidth(), view.minimumSizeHint().width());
            for (auto child : view.findChildren<QWidget *>())
                if (child->minimumSizeHint().width() > 310)
                    std::fprintf(stderr, "NARROW child=%s/%s min=%d hint=%d\n",
                                 child->metaObject()->className(), qPrintable(child->objectName()),
                                 child->minimumWidth(), child->minimumSizeHint().width());
        }
        check(view.width() <= 360,
              "manager reflows to a narrow dock rather than forcing an oversized panel");
        check(view.grab().save(snapshot + ".narrow.png"),
              "narrow manager screenshot writes successfully");
    }
    std::printf("color manager failures=%d\n", failures);
    return failures ? 1 : 0;
}
