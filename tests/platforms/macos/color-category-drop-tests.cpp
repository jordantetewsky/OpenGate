// SPDX-License-Identifier: MIT
#include "../../../src/core/colorcategoryeditor.h"
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include <memory>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool value, const char *message) {
        if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    Unbound::ColorCategoryEditor editor;
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    editor.resize(500, 600);
    Unbound::ColorPalette::Data data;
    data.categories = {{"a", "A"}, {"b", "B"}, {"c", "C"}};
    for (const auto &id : {"a1", "a2", "a3", "b1", "b2", "c1"}) {
        Unbound::ColorPreset paint;
        paint.id = paint.name = id;
        paint.category = QString(id).left(1);
        data.presets.append(paint);
    }
    int commits = 0;
    editor.changed = [&](const auto &next) {
        data = next; ++commits;
        editor.setData(data);
    };
    editor.setData(data);
    editor.show();
    app.processEvents();
    auto tree = editor.findChild<QTreeWidget *>("openGatePaletteCategories");
    auto item = [&](const QString &id) -> QTreeWidgetItem * {
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            if ((*it)->data(0, Qt::UserRole) == id) return *it;
            ++it;
        }
        return nullptr;
    };
    auto order = [&](const QString &category) {
        QStringList ids;
        for (const auto &paint : data.presets)
            if (paint.category == category) ids.append(paint.id);
        return ids.join(',');
    };
    auto drop = [&](QStringList ids, QString target, bool after = false) {
        QModelIndexList indexes;
        for (const auto &id : ids) {
            auto entry = item(id);
            const auto parent = entry->parent();
            const auto category = parent ? tree->indexOfTopLevelItem(parent) : tree->indexOfTopLevelItem(entry);
            const auto groupIndex = tree->model()->index(category, 0);
            indexes.append(parent ? tree->model()->index(parent->indexOfChild(entry), 0, groupIndex) : groupIndex);
        }
        std::unique_ptr<QMimeData> mime(tree->model()->mimeData(indexes));
        const auto rectangle = tree->visualItemRect(item(target));
        const auto point = QPoint(rectangle.center().x(), after ? rectangle.bottom()-1 : rectangle.top()+1);
        QDragEnterEvent enter(point, Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(tree->viewport(), &enter);
        QDragMoveEvent move(point, Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(tree->viewport(), &move);
        QDropEvent event(point, Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(tree->viewport(), &event);
        app.processEvents();
        return event.isAccepted();
    };
    check(drop({"a1"}, "a3", true) && order("a") == "a2,a3,a1",
          "downward swatch move uses the shown boundary without an off-by-one removal");
    check(drop({"a1"}, "a2") && order("a") == "a1,a2,a3",
          "upward swatch move inserts before the target");
    check(drop({"a1"}, "b") && order("a") == "a2,a3" && order("b") == "b1,b2,a1",
          "dropping on a category appends to that category without nesting a swatch");
    check(drop({"a"}, "b1") && data.categories[0].id == "b" && data.categories[1].id == "a" &&
              order("a") == "a2,a3" && tree->topLevelItemCount() == 3,
          "category dropped over another group's swatches moves after that whole group");
    check(drop({"b"}, "c", true) && data.categories[2].id == "b" && order("b") == "b1,b2,a1",
          "category reorder retains its swatches and never becomes another category's child");
    check(drop({"b2", "b1"}, "a3", true) && order("a") == "a2,a3,b1,b2" && order("b") == "a1",
          "multi-swatch move preserves visual source order rather than selection-list order");
    item("c")->setExpanded(false);
    check(drop({"a1"}, "c") && item("c")->isExpanded() && order("c") == "c1,a1",
          "drop into a collapsed category appends and reveals the moved swatch");
    item("b")->setExpanded(false);
    const auto before = Unbound::ColorPalette::encode(data);
    const int count = commits;
    check(!drop({"a", "a2"}, "c") && commits == count &&
              Unbound::ColorPalette::encode(data) == before,
          "mixed category/swatch selection is rejected before any tree or model mutation");
    editor.setData(data);
    check(!item("b")->isExpanded(), "palette refresh preserves unrelated collapsed categories");
    check(commits == 7 && data.presets.size() == 6 && data.categories.size() == 3,
          "each accepted drop commits once without losing presets or categories");
    if (!failures) std::puts("PASS: actual palette drag/drop events, ordering, category moves and rejection");
    return failures ? 1 : 0;
}
