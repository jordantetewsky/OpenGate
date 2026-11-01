// SPDX-License-Identifier: MIT
#pragma once
#include <QElapsedTimer>
#include <QThread>
#include "../../../src/core/panelscalecontrols.h"
namespace OpenGateTest {
template<class Check> void panelScaleControlChecks(Check check) {
    double value=1.;int applies=0;
    auto get=[&]{return value;};auto set=[&](double next){value=next;++applies;};
    QMenu direct;direct.setAttribute(Qt::WA_DontShowOnScreen);
    Unbound::populatePanelScaleMenu(&direct,&direct,get,set);
    QWidget host;Unbound::PanelScaleControls controls(&host,get,set);
    auto button=controls.findChild<QToolButton *>("openGatePanelScaleButton");
    auto title=button->menu();QMetaObject::invokeMethod(title,"aboutToShow");
    for(auto menu:{&direct,title}) {
        auto slider=menu->findChild<QSlider *>("openGatePanelScaleSlider");
        auto percent=menu->findChild<QSpinBox *>("openGatePanelScalePercent");
        check(slider && percent && slider->singleStep()==1,"both scale-menu entry points provide live 1-percent and typed controls");
        if(!slider || !percent)continue;
        const auto before=applies;
        for(int next=125;next<=263;++next)slider->setValue(next);
        check(applies==before && percent->value()==263,"drag bursts update the value display while coalescing scale application");
        QMetaObject::invokeMethod(slider,"sliderReleased");
        check(qAbs(value-2.63)<0.000001 && applies==before+1,"release applies the exact final intermediate percentage once");
        percent->setValue(147);QMetaObject::invokeMethod(menu,"aboutToHide");
        check(qAbs(value-1.47)<0.000001 && slider->value()==147,"typed percentage and menu dismissal flush the pending value");
        slider->setValue(263);QMetaObject::invokeMethod(slider,"sliderReleased");
        QAction *larger=nullptr,*reset=nullptr;
        for(auto action:menu->actions()) {if(action->text()=="Larger")larger=action;if(action->text()=="Reset to 100%")reset=action;}
        if(larger)larger->trigger();
        check(qAbs(value-2.73)<0.000001,"step controls preserve the entered fine percentage");
        slider->setValue(221);QMetaObject::invokeMethod(menu,"aboutToHide");if(reset)reset->trigger();
        QElapsedTimer elapsed;elapsed.start();
        while(elapsed.elapsed()<30){QApplication::processEvents();QThread::msleep(1);}
        check(value==1.,"a pending slider update cannot overwrite Reset after the menu closes");
    }
    QMenu retained;auto owner=new QObject;
    Unbound::populatePanelScaleMenu(&retained,owner,get,set);
    auto slider=retained.findChild<QSlider *>("openGatePanelScaleSlider");
    const auto before=applies;slider->setValue(213);delete owner;
    QMetaObject::invokeMethod(&retained,"aboutToHide");
    check(applies==before,"a native menu outliving its panel adapter cannot invoke a retired scale setter");
}
}
