// SPDX-License-Identifier: MIT
#pragma once
#include <QLayout>
#include <QPointer>
#include <QWidget>

namespace Unbound {
// Relax the workspace container, never the controls inside it. Native docking
// restores the exact policy and constraints captured before the transfer.
class PanelWidthAllowance
{
public:
    ~PanelWidthAllowance() { restore(); }
    void apply(QWidget *surface, int width = 240)
    {
        restore();
        if (!surface) return;
        widget = surface; minimum = surface->minimumSize(); policy = surface->sizePolicy();
        layout = surface->layout();
        if (layout) { constraint = layout->sizeConstraint(); layout->setSizeConstraint(QLayout::SetNoConstraint); }
        auto narrow = policy; narrow.setHorizontalPolicy(QSizePolicy::Ignored);
        surface->setSizePolicy(narrow);
        surface->setMinimumWidth(qMin(width, surface->maximumWidth()));
    }
    void restore()
    {
        if (widget) {
            if (layout) layout->setSizeConstraint(constraint);
            widget->setSizePolicy(policy); widget->setMinimumSize(minimum);
            widget->updateGeometry();
        }
        widget = nullptr; layout = nullptr;
    }
private:
    QPointer<QWidget> widget;
    QPointer<QLayout> layout;
    QSize minimum;
    QSizePolicy policy;
    QLayout::SizeConstraint constraint = QLayout::SetDefaultConstraint;
};
}
