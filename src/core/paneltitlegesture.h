// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QPointer>
#include <QLabel>
#include <QStyle>
#include <functional>
#include <utility>
namespace Unbound {
inline bool panelTitleHit(QWidget *title,QPoint point,const QString &nativeText={}) {
    if(!title || !title->rect().adjusted(2,2,-2,-2).contains(point))return false;
    auto label=qobject_cast<QLabel *>(title);
    if(label || !nativeText.isEmpty()) {
        const auto text=nativeText.isEmpty()?label->text():nativeText;
        const QSize ink(qMin(title->width(),title->fontMetrics().horizontalAdvance(text)+12),title->height());
        return QStyle::alignedRect(title->layoutDirection(),label?label->alignment():Qt::AlignLeft|Qt::AlignVCenter,ink,title->rect()).contains(point);
    }
    return true;
}
// Shared click/drag distinction, with no platform window APIs or layout policy.
// Native clicks pass through; only a held left-button threshold starts a drag.
class PanelTitleGesture : public QObject {
public:
    PanelTitleGesture(QWidget *title,QObject *owner,std::function<void(QPoint)> move,QString nativeText={})
        :QObject(owner),title(title),move(std::move(move)),nativeText(std::move(nativeText)) {title->installEventFilter(this);}
    ~PanelTitleGesture() override {if(title)title->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if(event->type()==QEvent::MouseButtonPress) {
            auto mouse=static_cast<QMouseEvent *>(event);armed=mouse->button()==Qt::LeftButton&&panelTitleHit(title,mouse->pos(),nativeText);dragged=false;press=mouse->globalPos();
        } else if(event->type()==QEvent::MouseMove && armed) {
            auto mouse=static_cast<QMouseEvent *>(event);
            if(mouse->buttons().testFlag(Qt::LeftButton) && (mouse->globalPos()-press).manhattanLength()>=QApplication::startDragDistance()) {
                armed=false;dragged=true;
                if(auto button=qobject_cast<QAbstractButton *>(title.data()))button->setDown(false);
                if(QWidget::mouseGrabber()==title)title->releaseMouse();
                if(move)move(mouse->globalPos());return true;
            }
        } else if(event->type()==QEvent::MouseButtonRelease) {armed=false;const bool consume=dragged;dragged=false;return consume;}
        else if(event->type()==QEvent::Hide || event->type()==QEvent::UngrabMouse)armed=false;
        return false;
    }
private:
    QPointer<QWidget> title;std::function<void(QPoint)> move;QString nativeText;QPoint press;bool armed=false,dragged=false;
};
}
