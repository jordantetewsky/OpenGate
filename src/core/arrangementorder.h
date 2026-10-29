// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QTimer>
#include <functional>
namespace Unbound {
class ArrangementOrder : public QListWidget
{
public:
    explicit ArrangementOrder(QWidget *owner) : QListWidget(owner)
    {
        setObjectName("arrangementOrder"); setAccessibleName("Workspace preset order");
        setDragDropMode(QAbstractItemView::InternalMove); setDefaultDropAction(Qt::MoveAction);
        setMinimumHeight(80); setMaximumHeight(180); setToolTip("Drag a grip to change the order in Workspace UI");
        timer.setSingleShot(true);
        connect(model(), &QAbstractItemModel::rowsMoved, this, [this] { timer.start(0); });
        connect(&timer, &QTimer::timeout, this, [this] {
            QStringList ids; for (int i = 0; i < count(); ++i) ids.append(item(i)->data(Qt::UserRole).toString());
            if (reordered) reordered(ids);
        });
    }
    void reload(const QJsonArray &entries)
    {
        timer.stop(); clear();
        for (const auto &value : entries) {
            const auto entry = value.toObject();
            auto row = new QListWidgetItem(QString::fromUtf8("⠿  ") + entry["name"].toString(), this);
            row->setData(Qt::UserRole, entry["id"].toString()); row->setSizeHint(QSize(0,28));
        }
    }
    std::function<void(QStringList)> reordered;
private:
    QTimer timer;
};
}
