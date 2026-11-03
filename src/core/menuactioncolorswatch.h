// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTimer>

namespace Unbound {
// A native action's right-side swatch only. Native QMenu retains row painting,
// hit testing, accessibility and activation; the overlay never receives input.
class MenuActionColorSwatch : public QObject {
public:
    MenuActionColorSwatch(QMenu *menu,QAction *action,QColor color):QObject(action),menu(menu),action(action),dot(new Dot(menu,color)) {
        menu->installEventFilter(this);connect(action,&QAction::changed,this,[this]{queue();});queue();
    }
    ~MenuActionColorSwatch() override {delete dot;}
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        switch(event->type()) {
        case QEvent::Paint:case QEvent::Resize:case QEvent::Show:case QEvent::Wheel:queue();break;
        case QEvent::Hide:if(dot)dot->hide();break;
        default:break;
        }
        return false;
    }
private:
    class Dot : public QWidget {
    public:
        Dot(QWidget *owner,QColor color):QWidget(owner),color(color) {
            setObjectName("openGateMenuColorSwatch");setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);setFocusPolicy(Qt::NoFocus);hide();
        }
    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(Qt::black,1));p.setBrush(color);
            const double size=qBound(6.,height()/3.,14.);p.drawEllipse(QRectF((width()-size)/2.,(height()-size)/2.,size,size));
        }
    private:QColor color;
    };
    QPointer<QMenu> menu;QPointer<QAction> action;QPointer<Dot> dot;bool queued=false;
    void queue() {
        if(queued)return;queued=true;QTimer::singleShot(0,this,[this]{
            queued=false;if(!menu || !action || !dot)return;
            const auto row=menu->actionGeometry(action);
            dot->setGeometry(row.right()-20-row.height()/2,row.top(),row.height(),row.height());
            dot->setVisible(menu->isVisible() && action->isVisible() && row.intersects(menu->rect()));dot->raise();
        });
    }
};
}
