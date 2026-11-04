// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QStandardItemModel>
#include <QTreeView>

// Reuse the real workflow wrapper, registry, bridge and event loop from the
// workflow fixture; only the editors and input sequence differ here.
template <typename Check, typename Settle, typename Publish>
void renameCommandContextChecks(QWidget &main, QAction *action, Unbound::PanelCommands &commands, int &nativeCalls,
                               int &registryCalls, Check check, Settle settle, Publish publish)
{
    QTreeView bins(&main);
    QStandardItemModel model;
    model.appendRow(new QStandardItem("Bin"));
    bins.setModel(&model);
    bins.show();
    bins.setFocus();
    bins.edit(model.index(0, 0));
    settle();
    auto editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
    check(editor != nullptr, "bin rename uses a focused native delegate editor");
    if (!editor) return;
    const auto nativeBefore = nativeCalls;
    const auto registryBefore = registryCalls;
    QShortcutEvent shortcut(QKeySequence("Shift+M"), 0, false);
    QApplication::sendEvent(action, &shortcut);
    settle();
    check(nativeCalls == nativeBefore && registryCalls == registryBefore,
          "OpenGate native shortcut cannot invoke a workflow handler during bin rename");

    editor->selectAll();
    QKeyEvent override(QEvent::ShortcutOverride, Qt::Key_M, Qt::ShiftModifier, "M");
    override.ignore();
    QApplication::sendEvent(editor, &override);
    check(override.isAccepted(), "Shift-letter keeps text editing priority over global shortcuts");
    QKeyEvent letter(QEvent::KeyPress, Qt::Key_M, Qt::ShiftModifier, "M");
    QApplication::sendEvent(editor, &letter);
    check(editor->text() == "M", "claimed Shift-letter still reaches the bin rename editor");

    const auto priorOwnership = commands.nativeOwnedShortcut;
    commands.nativeOwnedShortcut = [](const QKeySequence &key) {
        return key == QKeySequence("Ctrl+Shift+M") || key == QKeySequence("Ctrl+Z");
    };
    QKeyEvent owned(QEvent::ShortcutOverride, Qt::Key_M,
                    Qt::ControlModifier | Qt::ShiftModifier);
    owned.ignore();
    QApplication::sendEvent(editor, &owned);
    check(owned.isAccepted(), "modified OpenGate key is claimed independently of command availability");

    int nativeUndo = 0;
    QAction undo(&main);
    undo.setObjectName("nativeUndo");
    QObject::connect(&undo, &QAction::triggered, [&] { ++nativeUndo; });
    QShortcutEvent undoKey(QKeySequence("Ctrl+Z"), 0, false);
    QApplication::sendEvent(&undo, &undoKey);
    check(nativeUndo == 1, "native editing shortcut routing survives even an OpenGate key collision");
    commands.nativeOwnedShortcut = priorOwnership;

    const auto issuedWhileRenaming = QDateTime::currentMSecsSinceEpoch();
    settle();
    bins.hide();
    main.setFocus();
    settle();
    const auto beforeLate = registryCalls;
    publish("late-rename.json", issuedWhileRenaming);
    settle();
    check(registryCalls == beforeLate,
          "workflow request issued while renaming stays rejected after editor loses focus");
    publish("after-rename.json", QDateTime::currentMSecsSinceEpoch());
    settle();
    check(registryCalls == beforeLate + 1, "fresh workflow command works once after rename ends");

    QGraphicsScene scene;
    auto text = scene.addText("Clip");
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    QGraphicsView clips(&scene, &main);
    clips.show();
    clips.setFocus();
    text->setFocus();
    settle();
    check(Unbound::textEditor(QApplication::focusWidget()),
          "clip rename is recognized inside a graphics scene");
    const auto beforeClip = nativeCalls;
    QShortcutEvent clipKey(QKeySequence("Shift+M"), 0, false);
    QApplication::sendEvent(action, &clipKey);
    settle();
    check(nativeCalls == beforeClip, "OpenGate native shortcut is suppressed during clip rename");

    // Some native scenes toggle editability without changing QWidget focus.
    text->setTextInteractionFlags(Qt::NoTextInteraction);
    QKeyEvent idle(QEvent::KeyRelease, Qt::Key_F2, Qt::NoModifier);
    QApplication::sendEvent(clips.viewport(), &idle);
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    text->setFocus();
    QKeyEvent start(QEvent::ShortcutOverride, Qt::Key_M, Qt::ShiftModifier, "M");
    QApplication::sendEvent(clips.viewport(), &start);
    const auto issuedInScene = QDateTime::currentMSecsSinceEpoch();
    settle();
    text->setTextInteractionFlags(Qt::NoTextInteraction);
    clips.hide();
    main.setFocus();
    settle();
    const auto beforeSceneLate = registryCalls;
    publish("late-scene-rename.json", issuedInScene);
    settle();
    check(registryCalls == beforeSceneLate,
          "scene editability changes also guard workflow requests delivered after rename");
}
