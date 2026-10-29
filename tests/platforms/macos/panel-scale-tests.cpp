// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTemporaryDir>
#include <cstdio>
#include "panelmetricscale.h"
#include "panelsession.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *message) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    QWidget native,first,second; auto layout=new QVBoxLayout(&first); layout->setContentsMargins(10,12,14,16); layout->setSpacing(8);
    auto button=new QPushButton("Real native action",&first); button->setFixedSize(100,40); button->setIconSize({20,20});
    auto font=button->font(); font.setPointSizeF(12); button->setFont(font); layout->addWidget(button);
    const auto nativeFont=native.font(); const auto secondFont=second.font(); const auto original=button->font();
    Unbound::PanelMetricScale scale;
    check(scale.attach(&first,0.8),"scoped scale attaches");
    check(button->minimumSize()==QSize(80,32) && button->maximumSize()==QSize(80,32) && button->iconSize()==QSize(16,16),"fixed controls and icons scale together");
    check(qAbs(button->font().pointSizeF()-9.6)<0.01 && layout->spacing()==6,"font and layout metrics scale");
    check(native.font()==nativeFont && second.font()==secondFont,"other windows and native workspace remain untouched");
    int clicks=0; QObject::connect(button,&QPushButton::clicked,[&] { ++clicks; }); button->click(); check(clicks==1,"original action signal remains connected");
    scale.attach(&first,1.2); scale.attach(&first,0.8);
    check(button->minimumSize()==QSize(80,32),"repeated scaling derives from baseline without drift");
    for(double factor:{1.5,2.0,3.0}) {
        check(scale.attach(&first,factor),"larger scale attaches without a raster transform");
        check(qAbs(button->font().pointSizeF()-12*factor)<0.01&&button->iconSize()==QSize(qRound(20*factor),qRound(20*factor)),"native font and icon metrics retain full-size resolution through 300 percent");
        check(button->minimumSize()==QSize(qRound(100*factor),qRound(40*factor)),"native control geometry scales through 300 percent");
    }
    Unbound::MacPanelSession session;check(session.setInterfaceScale(3.0)&&session.interfaceScale()==3.0&&!session.setInterfaceScale(3.1),"session preserves 300 percent and rejects out-of-range values");
    scale.attach(&first,0.8);
    auto lazy=new QPushButton("Lazy control",&first); lazy->setFixedSize(50,30); QApplication::processEvents(); QApplication::processEvents();
    check(lazy->minimumSize()==QSize(40,24),"lazy native controls join the scoped scale");
    check(qAbs(lazy->font().pointSizeF()-first.font().pointSizeF())<0.01,"lazy inherited fonts are not scaled twice");
    delete lazy; QApplication::processEvents();
    check(!scale.attach(&first,0.2) && !scale.attach(&first,qQNaN()),"invalid factors cannot alter metrics");
    scale.restore(); scale.restore();
    check(button->minimumSize()==QSize(100,40) && button->maximumSize()==QSize(100,40) && button->font()==original && button->iconSize()==QSize(20,20),"native control metrics restore exactly");
    check(layout->contentsMargins()==QMargins(10,12,14,16) && layout->spacing()==8,"native layout metrics restore exactly");
    auto disposable=new QWidget; scale.attach(disposable,1.1); delete disposable; scale.restore();
    std::printf("panel scale failures=%d\n",failures); return failures ? 1 : 0;
}
