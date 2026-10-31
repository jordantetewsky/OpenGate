// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsProxyWidget>
namespace Unbound {
// Shared input guard; never reads or records editor contents.
inline bool textEditor(QWidget *widget)
{
    for (auto parent = widget; parent; parent = parent->parentWidget()) {
        if (qobject_cast<QLineEdit *>(parent) || qobject_cast<QTextEdit *>(parent) ||
            qobject_cast<QPlainTextEdit *>(parent) || qobject_cast<QAbstractSpinBox *>(parent)) return true;
        auto combo = qobject_cast<QComboBox *>(parent);
        if (combo && combo->isEditable()) return true;
        if (auto view=qobject_cast<QGraphicsView *>(parent)) if (view->scene()) {
            auto item=view->scene()->focusItem();
            auto text=qgraphicsitem_cast<QGraphicsTextItem *>(item);
            if (text && (text->textInteractionFlags() & Qt::TextEditable)) return true;
            auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
            if(proxy && proxy->widget()) {
                auto editor=proxy->widget()->focusWidget();
                if(editor && editor!=parent && textEditor(editor))return true;
            }
        }
        if (parent->isWindow()) break;
    }
    return false;
}

}
