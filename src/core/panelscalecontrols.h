// SPDX-License-Identifier: MIT
#pragma once
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QWidgetAction>
#include <functional>
namespace Unbound {
inline void populatePanelScaleMenu(QMenu *menu,QObject *owner,std::function<double()> value,std::function<void(double)> set) {
    menu->setProperty("unboundOwnMenu",true);
    const QPair<const char *,double> changes[]={{"Smaller",-0.1},{"Larger",0.1},{"Reset to 100%",0.0}};
    for (const auto &change:changes) {
        auto action=menu->addAction(QString::fromLatin1(change.first));
        QObject::connect(action,&QAction::triggered,owner,[value,set,delta=change.second] {
            set(delta==0.0 ? 1.0 : qBound(0.7,qRound((value()+delta)*10.0)/10.0,3.0));
        });
    }
    menu->addSeparator();
    for (int percent:{70,80,90,100,110,120,130,140,150,175,200,225,250,275,300}) {
        auto action=menu->addAction(QString::number(percent)+"%"); action->setCheckable(true);
        action->setChecked(qAbs(value()-percent/100.)<0.001);
        QObject::connect(action,&QAction::triggered,owner,[set,percent] { set(percent/100.); });
    }
}
class PanelScaleControls : public QWidget {
public:
    PanelScaleControls(QWidget *parent,std::function<double()> value,std::function<void(double)> set,
                       std::function<bool()> available=[] { return true; }) : QWidget(parent) {
        setObjectName("openGatePanelScale"); setProperty("openGateChrome",true);
        auto row=new QHBoxLayout(this); row->setContentsMargins(0,0,0,0); row->setSpacing(0);
        auto button=new QToolButton(this);button->setObjectName("openGatePanelScaleButton");
        button->setAutoRaise(true);button->setFocusPolicy(Qt::NoFocus);button->setPopupMode(QToolButton::InstantPopup);
        button->setToolTip("Panel interface size; custom-painted timeline tracks are not scaled");row->addWidget(button);
        auto update=[button,value] { button->setText(QString::number(qRound(value()*100))+"%"); };update();
        auto menu=new QMenu(button);menu->setProperty("unboundOwnMenu",true);button->setMenu(menu);
        connect(menu,&QMenu::aboutToShow,this,[menu,this,value,set,available,update] {
            update();menu->clear();menu->addSection("Panel interface size");
            auto holder=new QWidget(menu);auto layout=new QHBoxLayout(holder);auto slider=new QSlider(Qt::Horizontal,holder);
            slider->setRange(70,300);slider->setSingleStep(10);slider->setValue(qRound(value()*100));slider->setMinimumWidth(150);layout->addWidget(slider);
            auto action=new QWidgetAction(menu);action->setDefaultWidget(holder);menu->addAction(action);
            connect(slider,&QSlider::valueChanged,this,[set,available,update](int percent) { if (available()) { set(percent/100.);update(); } });
            populatePanelScaleMenu(menu,this,value,[set,available,update](double factor) { if (available()) { set(factor);update(); } });
        });
    }
};
}
