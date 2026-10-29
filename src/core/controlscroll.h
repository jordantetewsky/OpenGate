// SPDX-License-Identifier: MIT
#pragma once
#include <QScrollArea>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <functional>
#include "diagnostics.h"
namespace Unbound {
class ControlScroll : public QScrollArea
{
public:
    explicit ControlScroll(QWidget *owner) : QScrollArea(owner)
    {
        const auto record = [this] { logEvent(QStringLiteral("controls.scroll"), {{"x", horizontalScrollBar()->value()}, {"y", verticalScrollBar()->value()}, {"row_update", updating}}); };
        connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, record);
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, record);
    }
    void preservePosition(const std::function<void()> &update)
    {
        const int x = horizontalScrollBar()->value(), y = verticalScrollBar()->value();
        QScopedValueRollback<bool> guard(updating, true);
        update();
        horizontalScrollBar()->setValue(x); verticalScrollBar()->setValue(y);
    }
protected:
    bool focusNextPrevChild(bool next) override
    {
        // Disabling a clicked button transfers focus; it must not scroll the list.
        return updating ? QWidget::focusNextPrevChild(next) : QScrollArea::focusNextPrevChild(next);
    }
private:
    bool updating = false;
};
}
