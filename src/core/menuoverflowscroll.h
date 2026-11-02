// SPDX-License-Identifier: MIT
#pragma once
#include <QMenu>
#include <QPointer>
#include <QScrollBar>
#include <QScreen>
#include <QSignalBlocker>
#include <QScopedValueRollback>
#include <QTimer>
#include <climits>

namespace Unbound {
// Fit late-populated native menus and expose their native row scrolling through
// a visible scrollbar. Actions, activation and popup ownership stay with QMenu.
class MenuOverflowScroll : public QObject {
public:
    static void refresh(QMenu *menu) {
        if(!menu)return;
        auto owner=dynamic_cast<MenuOverflowScroll *>(menu->findChild<QObject *>("openGateMenuOverflow",Qt::FindDirectChildrenOnly));
        if(!owner)owner=new MenuOverflowScroll(menu);
        owner->arrange();
    }
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if(changing)return false;
        switch(event->type()) {
        case QEvent::ActionAdded:case QEvent::ActionRemoved:case QEvent::ActionChanged:
            fitNeeded=true;queue();break;
        case QEvent::Resize:case QEvent::Wheel:case QEvent::KeyRelease:case QEvent::MouseMove:queue();break;
        case QEvent::Hide:bar->hide();break;
        default:break;
        }
        return false;
    }
private:
    QPointer<QMenu> menu;QScrollBar *bar;
    bool changing=false,queued=false,fitNeeded=true;
    explicit MenuOverflowScroll(QMenu *menu):QObject(menu),menu(menu),bar(new QScrollBar(Qt::Vertical,menu)) {
        setObjectName("openGateMenuOverflow");bar->setObjectName("openGateMenuOverflowScroll");
        bar->setAccessibleName("Scroll menu items");bar->setFocusPolicy(Qt::NoFocus);
        bar->setStyleSheet("QScrollBar:vertical{background:#202124;width:16px;margin:0;}"
            "QScrollBar::handle:vertical{background:#60666e;min-height:24px;border-radius:4px;margin:2px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:none;}");
        bar->hide();menu->installEventFilter(this);
        connect(bar,&QScrollBar::valueChanged,this,[this](int value){
            if(changing || !this->menu)return;
            const auto rows=actions();if(rows.isEmpty())return;
            QAction *target=value==0?rows.front():rows.back();
            if(value>0 && value<bar->maximum()) {
                const int top=this->menu->actionGeometry(rows.front()).top();int distance=INT_MAX;
                for(auto action:rows) {
                    const int delta=qAbs(this->menu->actionGeometry(action).center().y()-top-value-this->menu->height()/2);
                    if(delta<distance){target=action;distance=delta;}
                }
            }
            {QScopedValueRollback<bool> guard(changing,true);this->menu->setActiveAction(target);}
            queue();
        });
    }
    QList<QAction *> actions() const {
        QList<QAction *> rows;
        if(menu)for(auto action:menu->actions())if(action->isVisible() && !action->isSeparator())rows.append(action);
        return rows;
    }
    void queue() {
        if(changing || queued)return;queued=true;
        QTimer::singleShot(0,this,[this]{queued=false;arrange();});
    }
    void arrange() {
        if(changing || !menu || !menu->isVisible() || !menu->screen())return;
        QScopedValueRollback<bool> guard(changing,true);
        const auto bounds=menu->screen()->availableGeometry();const auto rows=actions();
        int content=0;for(auto action:menu->actions())if(action->isVisible())content+=menu->actionGeometry(action).height();
        const bool overflow=content>qMin(bounds.height()-8,menu->maximumHeight());
        if(fitNeeded || !bounds.contains(menu->frameGeometry())) {
            // Re-run QMenu's public popup layout after late action insertion.
            // This initializes native screen bounds and native scroll flags too.
            fitNeeded=false;menu->popup(menu->pos());if(!menu)return;
        }
        // QMenu action rectangles retain their native sizeHint width; the extra
        // strip belongs only to the scrollbar, never to a native action/icon.
        menu->resize(qMin(bounds.width(),menu->sizeHint().width()+(overflow?18:0)),menu->height());
        menu->move(qBound(bounds.left(),menu->x(),qMax(bounds.left(),bounds.right()-menu->width()+1)),
                   qBound(bounds.top(),menu->y(),qMax(bounds.top(),bounds.bottom()-menu->height()+1)));
        const bool scroll=overflow && rows.size()>1;
        bar->setVisible(scroll);if(!scroll)return;
        const QSignalBlocker block(bar);
        bar->setRange(0,qMax(1,content-menu->height()));bar->setPageStep(menu->height());
        bar->setSingleStep(qMax(1,content/rows.size()));
        const auto first=menu->actionGeometry(rows.front()),last=menu->actionGeometry(rows.back());
        bar->setValue(first.top()>=0?0:last.bottom()<menu->height()-2?bar->maximum():qMax(0,-first.top()));
        bar->setGeometry(menu->width()-17,2,16,qMax(1,menu->height()-4));bar->raise();
    }
};
}
