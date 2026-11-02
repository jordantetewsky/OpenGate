// SPDX-License-Identifier: MIT
#pragma once
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QPointer>
#include <QWidgetAction>
#include <functional>
namespace Unbound {
inline void populatePanelScaleMenu(QMenu *menu,QObject *owner,std::function<double()> value,std::function<void(double)> set,double maximum=3.) {
    menu->setProperty("unboundOwnMenu",true);
    auto holder=new QWidget(menu);auto layout=new QHBoxLayout(holder);
    auto slider=new QSlider(Qt::Horizontal,holder);slider->setObjectName("openGatePanelScaleSlider");
    slider->setRange(70,qBound(70,qRound(maximum*100),300));slider->setSingleStep(1);slider->setPageStep(10);
    slider->setValue(qRound(value()*100));slider->setMinimumWidth(180);
    slider->setAccessibleName("Panel content size");layout->addWidget(slider);
    auto percent=new QSpinBox(holder);percent->setObjectName("openGatePanelScalePercent");
    percent->setRange(70,slider->maximum());percent->setSingleStep(1);percent->setSuffix("%");
    percent->setKeyboardTracking(false);percent->setValue(slider->value());
    percent->setAccessibleName("Panel content size in percent");layout->addWidget(percent);
    auto action=new QWidgetAction(menu);action->setDefaultWidget(holder);menu->addAction(action);
    auto pending=new QTimer(holder);pending->setSingleShot(true);pending->setInterval(16);
    auto apply=[pending,slider,value,set,alive=QPointer<QObject>(owner)] {
        pending->stop();if(!alive)return;
        if(qAbs(value()-slider->value()/100.)>0.000001)set(slider->value()/100.);
    };
    QObject::connect(pending,&QTimer::timeout,holder,apply);
    QObject::connect(slider,&QSlider::valueChanged,holder,[percent,pending](int next) {
        percent->setValue(next);if(!pending->isActive())pending->start();
    });
    QObject::connect(percent,QOverload<int>::of(&QSpinBox::valueChanged),slider,&QSlider::setValue);
    auto flush=[pending,apply] { if(pending->isActive())apply(); };
    QObject::connect(slider,&QSlider::sliderReleased,holder,flush);
    QObject::connect(menu,&QMenu::aboutToHide,holder,flush);
    menu->addSeparator();
    const QPair<const char *,double> changes[]={{"Smaller",-0.1},{"Larger",0.1},{"Reset to 100%",0.0}};
    for (const auto &change:changes) {
        auto action=menu->addAction(QString::fromLatin1(change.first));
        QObject::connect(action,&QAction::triggered,owner,[value,set,maximum,delta=change.second] {
            set(delta==0.0 ? qMin(1.,maximum) : qBound(0.7,qRound((value()+delta)*100.0)/100.0,maximum));
        });
    }
    menu->addSeparator();
    for (int percent:{70,80,90,100,110,120,130,140,150,175,200,225,250,275,300}) {
        auto action=menu->addAction(QString::number(percent)+"%"); action->setCheckable(true);
        action->setChecked(qAbs(value()-percent/100.)<0.001);
        action->setEnabled(percent<=qRound(maximum*100));
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
        button->setToolTip("Content size for this panel");row->addWidget(button);
        auto update=[button,value] { button->setText(QString::number(qRound(value()*100))+"%"); };update();
        auto menu=new QMenu(button);menu->setProperty("unboundOwnMenu",true);button->setMenu(menu);
        connect(menu,&QMenu::aboutToShow,this,[menu,this,value,set,available,update] {
            update();menu->clear();menu->addSection("Panel content size");
            populatePanelScaleMenu(menu,this,value,[set,available,update](double factor) { if (available()) { set(factor);update(); } });
        });
    }
};
}
