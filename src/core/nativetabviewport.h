// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QBoxLayout>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTimer>
#include <QWheelEvent>
namespace Unbound {
// Experimental continuous scrolling through Qt's public viewport model. Native
// tabs still paint, receive clicks, reorder and emit their original signals.
// The scrollbar owns a separate layout row below the native tab row.
class NativeTabViewport : public QObject {
public:
    explicit NativeTabViewport(QObject *parent):QObject(parent) {
        timer.setSingleShot(true);connect(&timer,&QTimer::timeout,this,[this]{fit();});
    }
    ~NativeTabViewport() override {restore();}
    bool active()const{return area && tabs;}
    bool attach(QTabBar *bar) {
        if(active() || !bar || !bar->parentWidget())return false;
        auto nativeParent=bar->parentWidget();auto slot=nativeParent->layout();
        if(!slot || slot->indexOf(bar)<0)return false;
        QWidget *row=bar;QBoxLayout *vertical=nullptr;
        for(int depth=0;depth<4 && row->parentWidget();++depth,row=row->parentWidget()) {
            auto candidate=qobject_cast<QBoxLayout *>(row->parentWidget()->layout());
            if(candidate && candidate->direction()==QBoxLayout::TopToBottom && candidate->indexOf(row)>=0){vertical=candidate;break;}
        }
        if(!vertical)return false;
        auto bandOwner=row->parentWidget();const int rowIndex=vertical->indexOf(row);
        tabs=bar;originalParent=nativeParent;originalLayout=slot;rowLayout=vertical;
        oldGeometry=bar->geometry();oldPolicy=bar->sizePolicy();oldFill=bar->autoFillBackground();oldArrows=bar->usesScrollButtons();oldElide=bar->elideMode();oldExpanding=bar->expanding();
        area=new QScrollArea(nativeParent);area->setObjectName("openGateTimelineTabViewport");area->setProperty("openGateMetricContainer",true);area->setFrameShape(QFrame::NoFrame);area->setMinimumWidth(0);area->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        area->setWidgetResizable(false);area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto oldItem=slot->replaceWidget(bar,area,Qt::FindDirectChildrenOnly);
        if(!oldItem){delete area.data();area=nullptr;tabs=nullptr;return false;}delete oldItem;
        area->setWidget(bar);bar->setAutoFillBackground(oldFill);bar->setUsesScrollButtons(false);bar->setExpanding(false);bar->setElideMode(Qt::ElideNone);
        band=new QWidget(bandOwner);band->setObjectName("openGateTimelineTabScrollBand");band->setProperty("openGateChrome",true);
        auto padding=new QVBoxLayout(band);padding->setContentsMargins(0,3,0,2);padding->setSpacing(0);
        scroll=new QScrollBar(Qt::Horizontal,band);scroll->setObjectName("openGateTimelineTabScroll");scroll->setFixedHeight(7);scroll->setFocusPolicy(Qt::NoFocus);scroll->setToolTip("Scroll timeline tabs");padding->addWidget(scroll);
        vertical->insertWidget(rowIndex+1,band);
        auto model=area->horizontalScrollBar();
        connect(scroll,&QScrollBar::valueChanged,this,[model](int value){model->setValue(value);});
        connect(model,&QScrollBar::valueChanged,this,[this](int value){if(scroll){QSignalBlocker block(scroll);scroll->setValue(value);}});
        connect(model,&QScrollBar::rangeChanged,this,[this](int,int){schedule();});
        connect(bar,&QTabBar::currentChanged,this,[this](int index){schedule();QTimer::singleShot(0,this,[this,index]{if(area&&tabs&&index>=0&&index<tabs->count())area->ensureVisible(tabs->tabRect(index).center().x(),0,20,0);});});
        bar->installEventFilter(this);area->installEventFilter(this);area->viewport()->installEventFilter(this);bar->show();area->show();schedule();return true;
    }
    bool restore() {
        timer.stop();
        if(!area)return true;
        if(!tabs)return false;
        if(!originalParent || !originalLayout || originalParent->layout()!=originalLayout || originalLayout->indexOf(area)<0 || area->widget()!=tabs)return false;
        tabs->removeEventFilter(this);area->removeEventFilter(this);area->viewport()->removeEventFilter(this);
        auto widget=area->takeWidget();auto item=originalLayout->replaceWidget(area,widget,Qt::FindDirectChildrenOnly);if(!item){area->setWidget(widget);return false;}delete item;
        widget->setParent(originalParent);tabs->setUsesScrollButtons(oldArrows);tabs->setExpanding(oldExpanding);tabs->setElideMode(oldElide);tabs->setSizePolicy(oldPolicy);tabs->setAutoFillBackground(oldFill);tabs->setGeometry(oldGeometry);tabs->show();
        if(rowLayout&&band)rowLayout->removeWidget(band);delete band.data();delete area.data();band=nullptr;area=nullptr;scroll=nullptr;tabs=nullptr;return true;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override {
        if(object==tabs && event->type()==QEvent::Wheel && area) {
            auto wheel=static_cast<QWheelEvent *>(event);if(wheel->modifiers()!=Qt::NoModifier)return false;
            const auto pixels=wheel->pixelDelta(),angles=wheel->angleDelta();const int delta=!pixels.isNull()?(pixels.x()?pixels.x():pixels.y()):(angles.x()?angles.x():angles.y())/3;
            auto model=area->horizontalScrollBar();if(delta&&model->maximum()>model->minimum()){model->setValue(model->value()-delta);event->accept();return true;}
        }
        if((object==tabs && event->type()==QEvent::Paint) || event->type()==QEvent::Resize || event->type()==QEvent::LayoutRequest || event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange || event->type()==QEvent::Show)schedule();
        return false;
    }
private:
    void schedule(){if(!timer.isActive())timer.start(0);}
    void fit() {
        if(!active())return;
        auto hint=tabs->sizeHint();if(!hint.isValid())return;
        const int height=qMax(tabs->minimumHeight(),hint.height());
        const QSize size(qMax(hint.width(),area->viewport()->width()),height);if(tabs->size()!=size)tabs->resize(size);
        if(area->minimumHeight()!=height || area->maximumHeight()!=height)area->setFixedHeight(height);
        auto model=area->horizontalScrollBar();
        {QSignalBlocker block(scroll);scroll->setRange(model->minimum(),model->maximum());scroll->setPageStep(model->pageStep());scroll->setValue(model->value());}
        band->setVisible(model->maximum()>model->minimum());
    }
    QPointer<QTabBar> tabs;QPointer<QWidget> originalParent,band;QPointer<QLayout> originalLayout;QPointer<QBoxLayout> rowLayout;
    QPointer<QScrollArea> area;QPointer<QScrollBar> scroll;QTimer timer;QRect oldGeometry;QSizePolicy oldPolicy;Qt::TextElideMode oldElide=Qt::ElideRight;bool oldArrows=true,oldExpanding=true,oldFill=false;
};
}
