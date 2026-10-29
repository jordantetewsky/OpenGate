// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QPainter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QToolButton>
namespace Unbound {
// Paint through the original native widget at its logical size. No replacement
// icon artwork, screenshot enlargement, font substitution or native reparenting.
class NativePageButton : public QToolButton {
public:
    explicit NativePageButton(QWidget *parent):QToolButton(parent) {setAutoRaise(true);setFocusPolicy(Qt::NoFocus);setCheckable(true);}
    void setNativeTarget(QAbstractButton *button) {target=button;updateGeometry();update();}
    QSize sizeHint() const override {
        if(!target)return QToolButton::sizeHint();
        auto size=target->size();if(size.width()<=0 || size.height()<=0)size=target->sizeHint();
        return size.expandedTo(QSize(24,20));
    }
protected:
    void paintEvent(QPaintEvent *) override {
        if(!target || rendering)return;
        QScopedValueRollback<bool> guard(rendering,true);QPainter painter(this);
        // QWidget render uses the target's own paint/style implementation. The
        // painter retains the destination's device scale; nothing is stretched.
        target->render(&painter,QPoint(qMax(0,(width()-target->width())/2),qMax(0,(height()-target->height())/2)),QRegion(target->rect()),QWidget::DrawWindowBackground|QWidget::DrawChildren);
        if(underMouse() && isEnabled())painter.fillRect(rect(),QColor(255,255,255,isDown()?22:10));
    }
private:
    QPointer<QAbstractButton> target;bool rendering=false;
};
}
