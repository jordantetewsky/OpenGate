// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <functional>
#include "workspacepages.h"
#include "nativepagebutton.h"
namespace Unbound {
struct NavigationPage { QString id,title;QPointer<QAbstractButton> target;bool current=false; };
// OpenGate controls mirror native page buttons. The native footer never moves.
class SurfacePageNavigation : public QScrollArea {
public:
    explicit SurfacePageNavigation(QWidget *host):QScrollArea(host) {
        setObjectName("openGateSurfacePageNavigation");setFrameShape(QFrame::NoFrame);setMinimumWidth(0);
        setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);setWidgetResizable(true);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        content=new QWidget;row=new QHBoxLayout(content);row->setContentsMargins(0,0,0,0);row->setSpacing(0);setWidget(content);
        timer.setSingleShot(true);connect(&timer,&QTimer::timeout,this,[this]{fit();});
        connect(horizontalScrollBar(),&QScrollBar::rangeChanged,this,[this]{schedule();});
        setToolTip("Page Navigation — scroll horizontally if all pages do not fit");
    }
    std::function<void(QString)> activate;
    void updatePages(const QList<NavigationPage> &pages) {
        QStringList next;for(const auto &page:pages)next.append(page.id);
        if(next!=ids) {
            while(auto item=row->takeAt(0)){delete item->widget();delete item;}buttons.clear();ids=next;
            for(const auto &page:pages) {
                auto button=new NativePageButton(content);button->setObjectName("openGatePage_"+page.id);button->setAutoRaise(true);button->setCheckable(true);button->setFocusPolicy(Qt::NoFocus);button->setIconSize(QSize(18,18));
                row->addWidget(button);buttons.append(button);
                connect(button,&QToolButton::clicked,this,[this,id=page.id]{if(activate)activate(id);});
            }
        }
        for(int i=0;i<pages.size();++i) {
            const auto &page=pages[i];auto button=buttons[i];button->setText(page.title);button->setToolTip(page.title);button->setAccessibleName(page.title);
            button->setEnabled(page.target && page.target->isEnabled());button->setChecked(page.current);
            button->setNativeTarget(page.target);
        }
        schedule();
    }
protected:
    void resizeEvent(QResizeEvent *event) override {QScrollArea::resizeEvent(event);schedule();}
    void changeEvent(QEvent *event) override {QScrollArea::changeEvent(event);if(event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange)schedule();}
private:
    void schedule(){if(!timer.isActive())timer.start(0);}
    void fit() {
        int labeled=0;for(auto button:buttons)labeled+=button->sizeHint().width()+row->spacing();
        const bool labels=viewport()->width()>=labeled+(compact?24:0);compact=!labels;
        int width=0,height=0;
        for(auto button:buttons) {
            button->setToolButtonStyle(labels?Qt::ToolButtonTextUnderIcon:Qt::ToolButtonIconOnly);
            const auto hint=button->sizeHint();const int buttonHeight=hint.height();
            button->setFixedSize(hint.width(),buttonHeight);
            width+=button->width()+row->spacing();height=qMax(height,button->height());
        }
        // Explicit content extent keeps every button reachable; Qt must scroll
        // rather than compress native labels or hide the final page.
        content->setMinimumSize(width,height);row->setAlignment(Qt::AlignCenter);
        const int scroll=width>viewport()->width()?horizontalScrollBar()->sizeHint().height():0;
        const int desired=height+scroll;if(minimumHeight()!=desired || maximumHeight()!=desired)setFixedHeight(desired);
    }
    QWidget *content;QHBoxLayout *row;QList<NativePageButton *> buttons;QStringList ids;QTimer timer;bool compact=false;
};
}
