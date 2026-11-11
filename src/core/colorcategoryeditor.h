// SPDX-License-Identifier: MIT
#pragma once
#include "colorpalette.h"
#include "colorcategorytree.h"
#include "colorpresetpaint.h"
#include <QDropEvent>
#include <QInputDialog>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
namespace Unbound {
// Palette category/list interaction only. Emits proposed ordering/category
// changes; the shared palette remains the sole committed state owner.
class ColorCategoryEditor : public QWidget {
  public:
    std::function<void(QString)> selected;
    std::function<void(const ColorPalette::Data &)> changed;
    explicit ColorCategoryEditor(QWidget *owner = nullptr) : QWidget(owner) {
        auto box = new QVBoxLayout(this);
        box->setContentsMargins(4, 4, 4, 4);
        box->setSpacing(8);
        tree = new ColorCategoryTree(this);
        tree->setHeaderHidden(true);
        tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tree->setIconSize(QSize(34, 14));
        tree->setDragDropMode(QAbstractItemView::InternalMove);
        tree->setDefaultDropAction(Qt::MoveAction);
        tree->setObjectName("openGatePaletteCategories");
        box->addWidget(tree);
        auto buttons = new QHBoxLayout;
        auto add = new QPushButton("New Category"), rename = new QPushButton("Rename"),
             remove = new QPushButton("Remove Category");
        box->addWidget(add);
        buttons->addWidget(rename);
        buttons->addWidget(remove);
        box->addLayout(buttons);
        connect(tree, &QTreeWidget::currentItemChanged, this,
                [this](QTreeWidgetItem *item, QTreeWidgetItem *) {
                    if (item && item->parent() && selected)
                        selected(item->data(0, Qt::UserRole).toString());
                });
        tree->dropped = [this] {
            auto next = model;
            next.categories.clear();
            next.presets.clear();
            bool valid = true;
            for (int i = 0; i < tree->topLevelItemCount(); ++i) {
                auto category = tree->topLevelItem(i);
                const auto id = category->data(0, Qt::UserRole).toString();
                bool found = false;
                for (const auto &c : model.categories)
                    if (c.id == id) {
                        next.categories.append(c);
                        found = true;
                    }
                valid &= found;
                for (int j = 0; j < category->childCount(); ++j) {
                    auto item = category->child(j);
                    if (item->childCount()) {
                        valid = false;
                        continue;
                    }
                    bool known = false;
                    for (auto p : model.presets)
                        if (p.id == item->data(0, Qt::UserRole).toString()) {
                            p.category = id;
                            next.presets.append(p);
                            known = true;
                        }
                    valid &= known;
                }
            }
            if (valid && next.presets.size() == model.presets.size() &&
                next.categories.size() == model.categories.size() && changed)
                changed(next);
            else
                setData(model, currentId());
        };
        connect(add, &QPushButton::clicked, this, [this] {
            if (model.categories.size() >= 32)
                return;
            bool ok = false;
            const auto text =
                QInputDialog::getText(this, "New category", "Name", QLineEdit::Normal, {}, &ok)
                    .trimmed();
            if (!ok || text.isEmpty() || text.size() > 80)
                return;
            auto next = model;
            next.categories.append({ColorPalette::newId(), text});
            if (changed)
                changed(next);
        });
        connect(rename, &QPushButton::clicked, this, [this] {
            const auto id = categoryId();
            if (id.isEmpty())
                return;
            auto next = model;
            for (auto &c : next.categories)
                if (c.id == id) {
                    bool ok = false;
                    const auto text = QInputDialog::getText(this, "Rename category", "Name",
                                                            QLineEdit::Normal, c.name, &ok)
                                          .trimmed();
                    if (!ok || text.isEmpty() || text.size() > 80)
                        return;
                    c.name = text;
                    if (changed)
                        changed(next);
                    return;
                }
        });
        connect(remove, &QPushButton::clicked, this, [this] {
            const auto id = categoryId();
            if (id.isEmpty())
                return;
            auto next = model;
            for (int i = 0; i < next.categories.size(); ++i)
                if (next.categories[i].id == id) {
                    next.categories.removeAt(i);
                    break;
                }
            if (next.categories.isEmpty())
                next.categories.append(ColorPalette::Category{"custom", "Your Presets"});
            for (auto &p : next.presets)
                if (p.category == id)
                    p.category = next.categories.front().id;
            if (changed)
                changed(next);
        });
    }
    void setData(const ColorPalette::Data &data, const QString &selection = {}) {
        QHash<QString, bool> expanded;
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            const auto item = tree->topLevelItem(i);
            expanded.insert(item->data(0, Qt::UserRole).toString(), item->isExpanded());
        }
        const auto current = tree->currentItem();
        const auto selectedId = selection.isEmpty() && current
            ? current->data(0, Qt::UserRole).toString() : selection;
        model = data;
        const QSignalBlocker blocked(tree);
        tree->clear();
        for (const auto &c : data.categories) {
            auto group = new QTreeWidgetItem(tree, {c.name});
            group->setIcon(0, gripIcon());
            group->setToolTip(0, "Drag to reorder this category and its swatches");
            group->setData(0, Qt::UserRole, c.id);
            if (c.id == selectedId) tree->setCurrentItem(group);
            group->setFlags(group->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            for (const auto &p : data.presets)
                if (p.category == c.id) {
                    auto row = new QTreeWidgetItem(group, {p.name});
                    row->setData(0, Qt::UserRole, p.id);
                    row->setFlags((row->flags() | Qt::ItemIsDragEnabled) & ~Qt::ItemIsDropEnabled);
                    row->setIcon(0, gripIcon(&p));
                    row->setToolTip(0, "Drag to reorder or move this swatch to another category");
                    if (p.id == selectedId)
                        tree->setCurrentItem(row);
                }
            group->setExpanded(expanded.value(c.id, true));
        }
    }

  private:
    ColorCategoryTree *tree;
    ColorPresetPaint paint;
    QIcon gripIcon(const ColorPreset *preset = nullptr) {
        QPixmap icon(34, 14);
        icon.fill(Qt::transparent);
        QPainter painter(&icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(142, 142, 150));
        for (int x : {3, 7})
            for (int y : {3, 7, 11})
                painter.drawEllipse(QPointF(x, y), .8, .8);
        if (preset)
            paint.fill(painter, QRectF(13, 1, 20, 12), *preset);
        return QIcon(icon);
    }
    ColorPalette::Data model;
    QString currentId() const {
        auto item = tree->currentItem();
        return item && item->parent() ? item->data(0, Qt::UserRole).toString() : QString();
    }
    QString categoryId() const {
        auto item = tree->currentItem();
        if (!item)
            return {};
        return (item->parent() ? item->parent() : item)->data(0, Qt::UserRole).toString();
    }
};
} // namespace Unbound
