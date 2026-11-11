// SPDX-License-Identifier: MIT
#pragma once
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>
#include <QPainter>
#include <QTreeWidget>
#include <QUuid>
#include <functional>

namespace Unbound {
// Palette-only two-level moves. Preview and commit share the same destination;
// Qt's general tree nesting/removal policy is never used for palette drops.
class ColorCategoryTree : public QTreeWidget {
  public:
    explicit ColorCategoryTree(QWidget *owner = nullptr) : QTreeWidget(owner) {
        setDropIndicatorShown(false);
        setAutoExpandDelay(600);
    }
    std::function<void()> dropped;

  protected:
    QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const override {
        auto mime = new QMimeData;
        QJsonArray ids;
        for (auto item : items)
            ids.append(item->data(0, Qt::UserRole).toString());
        mime->setData(mimeType(), QJsonDocument(QJsonObject{{"tree", token}, {"ids", ids}})
                                      .toJson(QJsonDocument::Compact));
        return mime;
    }
    QStringList mimeTypes() const override { return {mimeType()}; }
    void startDrag(Qt::DropActions) override {
        const auto items = movingItems(selectedItems());
        if (items.isEmpty()) return;
        QDrag drag(this);
        drag.setMimeData(mimeData(items));
        QPixmap badge(qMin(viewport()->width(), 280), 30);
        badge.fill(palette().color(QPalette::Base));
        QPainter painter(&badge);
        painter.setPen(palette().color(QPalette::Text));
        const auto text = items.size() == 1 ? items.front()->text(0)
            : QString::number(items.size()) + (items.front()->parent() ? " swatches" : " categories");
        painter.drawText(badge.rect().adjusted(8, 0, -8, 0), Qt::AlignVCenter, text);
        painter.end();
        drag.setPixmap(badge);
        drag.setHotSpot(QPoint(12, 15));
        drag.exec(Qt::MoveAction, Qt::MoveAction);
        clearDestination();
    }
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (!decode(event->mimeData()).isEmpty()) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else event->ignore();
    }
    void dragMoveEvent(QDragMoveEvent *event) override {
        // Let Qt maintain auto-scroll, then replace its unrestricted tree target.
        QTreeWidget::dragMoveEvent(event);
        destination = locate(event->pos(), decode(event->mimeData()));
        viewport()->update();
        if (destination.valid) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else event->ignore();
    }
    void dragLeaveEvent(QDragLeaveEvent *event) override {
        clearDestination();
        QTreeWidget::dragLeaveEvent(event);
    }
    void dropEvent(QDropEvent *event) override {
        const auto items = decode(event->mimeData());
        const auto target = locate(event->pos(), items);
        clearDestination();
        if (!target.valid) { event->ignore(); return; }
        // target.index refers to the pre-removal list. Adjust once for selected
        // siblings before it, preserving their visual order for multi-selection.
        int index = target.index;
        for (auto item : items)
            if (item->parent() == target.parent && siblingIndex(item) < target.index)
                --index;
        for (auto item : items) {
            if (auto parent = item->parent()) parent->takeChild(parent->indexOfChild(item));
            else takeTopLevelItem(indexOfTopLevelItem(item));
        }
        for (auto item : items) {
            if (target.parent) target.parent->insertChild(index++, item);
            else insertTopLevelItem(index++, item);
            item->setSelected(true);
        }
        if (target.parent) target.parent->setExpanded(true);
        event->setDropAction(Qt::MoveAction);
        event->accept();
        // The consumer may rebuild all items. Do not dereference them afterward.
        if (dropped) dropped();
    }
    void paintEvent(QPaintEvent *event) override {
        QTreeWidget::paintEvent(event);
        if (!destination.valid) return;
        QPainter painter(viewport());
        const auto color = palette().color(QPalette::Highlight);
        if (!destination.group.isEmpty()) {
            auto tint = color; tint.setAlpha(45);
            painter.fillRect(destination.group, tint);
        }
        painter.setPen(QPen(color, 3));
        painter.drawLine(destination.line.left(), destination.line.top(),
                         destination.line.right(), destination.line.top());
        painter.setBrush(color);
        painter.drawEllipse(QPointF(destination.line.left(), destination.line.top()), 3, 3);
    }

  private:
    struct Destination {
        bool valid = false;
        QTreeWidgetItem *parent = nullptr;
        int index = 0;
        QRect line, group;
    } destination;
    const QString token = QUuid::createUuid().toString();
    static QString mimeType() { return "application/x-opengate-palette-order"; }
    int siblingIndex(QTreeWidgetItem *item) const {
        return item->parent() ? item->parent()->indexOfChild(item) : indexOfTopLevelItem(item);
    }
    QList<QTreeWidgetItem *> movingItems(const QList<QTreeWidgetItem *> &selection) const {
        if (selection.isEmpty()) return {};
        const bool categories = !selection.front()->parent();
        for (auto item : selection)
            if ((!item->parent()) != categories) return {}; // No mixed nesting semantics.
        QList<QTreeWidgetItem *> ordered;
        for (int i = 0; i < topLevelItemCount(); ++i) {
            auto group = topLevelItem(i);
            if (categories && selection.contains(group)) ordered.append(group);
            if (!categories)
                for (int j = 0; j < group->childCount(); ++j)
                    if (selection.contains(group->child(j))) ordered.append(group->child(j));
        }
        return ordered;
    }
    QList<QTreeWidgetItem *> decode(const QMimeData *mime) const {
        if (!mime || mime->data(mimeType()).size() > 32768) return {};
        const auto object = QJsonDocument::fromJson(mime->data(mimeType())).object();
        if (object["tree"].toString() != token) return {};
        const auto ids = object["ids"].toArray();
        if (ids.isEmpty() || ids.size() > 160) return {};
        QSet<QString> wanted;
        for (const auto &id : ids) {
            if (!id.isString() || id.toString().isEmpty() || wanted.contains(id.toString())) return {};
            wanted.insert(id.toString());
        }
        QList<QTreeWidgetItem *> found;
        QTreeWidgetItemIterator it(const_cast<ColorCategoryTree *>(this));
        while (*it) {
            if (wanted.contains((*it)->data(0, Qt::UserRole).toString())) found.append(*it);
            ++it;
        }
        return found.size() == wanted.size() ? movingItems(found) : QList<QTreeWidgetItem *>{};
    }
    Destination locate(QPoint position, const QList<QTreeWidgetItem *> &items) const {
        Destination result;
        if (items.isEmpty() || !topLevelItemCount()) return result;
        auto under = itemAt(position);
        if (!items.front()->parent()) {
            // Hovering anywhere over a group's children means after that group;
            // categories can never become children of categories or swatches.
            auto group = under && under->parent() ? under->parent() : under;
            result.index = group ? indexOfTopLevelItem(group) : topLevelItemCount();
            int y = group ? visualItemRect(group).top() : viewport()->height() - 3;
            if (group && (under != group || position.y() >= visualItemRect(group).center().y())) {
                ++result.index;
                y = visualItemRect(group).bottom() + 1;
                if (group->isExpanded() && group->childCount())
                    y = visualItemRect(group->child(group->childCount()-1)).bottom() + 1;
            } else if (!group) {
                auto last = topLevelItem(topLevelItemCount()-1);
                if (last->isExpanded() && last->childCount()) last = last->child(last->childCount()-1);
                y = visualItemRect(last).bottom() + 1;
            }
            result.line = QRect(4, y, viewport()->width()-8, 1);
        } else {
            auto group = under ? (under->parent() ? under->parent() : under)
                               : topLevelItem(topLevelItemCount()-1);
            result.parent = group;
            result.index = under && under->parent() ? group->indexOfChild(under) : group->childCount();
            int y = visualItemRect(group).bottom() + 1;
            if (under && under->parent()) {
                y = visualItemRect(under).top();
                if (position.y() >= visualItemRect(under).center().y()) {
                    ++result.index; y = visualItemRect(under).bottom()+1;
                }
            } else if (group->isExpanded() && group->childCount())
                y = visualItemRect(group->child(group->childCount()-1)).bottom()+1;
            if (!under || !under->parent()) result.group = visualItemRect(group);
            result.line = QRect(indentation()+4, y, viewport()->width()-indentation()-8, 1);
        }
        result.valid = true;
        return result;
    }
    void clearDestination() { destination = {}; viewport()->update(); }
};
} // namespace Unbound
