#include <QApplication>
#include <QPushButton>
#include <cstdio>
#include "../../../src/core/paneltitlegesture.h"
#include "../../../src/core/panelhovertarget.h"
#include "nativepaneltitledrags.h"
int main(int argc,char **argv){
 QApplication app(argc,argv);QPushButton title("Media Pool");title.setAttribute(Qt::WA_DontShowOnScreen);title.show();app.processEvents();int clicks=0,drags=0;
 QObject::connect(&title,&QPushButton::clicked,[&]{++clicks;});Unbound::PanelTitleGesture gesture(&title,&title,[&](QPoint){++drags;});
 auto send=[&](QEvent::Type type,QPoint pos,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,pos,title.mapToGlobal(pos),button,buttons,Qt::NoModifier);QApplication::sendEvent(&title,&e);};
 send(QEvent::MouseButtonPress,{5,5},Qt::LeftButton,Qt::LeftButton);send(QEvent::MouseButtonRelease,{5,5},Qt::LeftButton,Qt::NoButton);
 if(clicks!=1||drags)return 1;
 send(QEvent::MouseButtonPress,{5,5},Qt::LeftButton,Qt::LeftButton);send(QEvent::MouseMove,{80,5},Qt::NoButton,Qt::LeftButton);send(QEvent::MouseButtonRelease,{80,5},Qt::LeftButton,Qt::NoButton);
 if(clicks!=1||drags!=1||title.isDown())return 2;
 QLabel label("Mixer");label.setAttribute(Qt::WA_DontShowOnScreen);label.resize(400,32);label.show();
 int labelDrags=0;Unbound::PanelTitleGesture labelGesture(&label,&label,[&](QPoint){++labelDrags;});
 auto labelEvent=[&](QEvent::Type type,QPoint pos,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,pos,label.mapToGlobal(pos),button,buttons,Qt::NoModifier);QApplication::sendEvent(&label,&e);};
 labelEvent(QEvent::MouseButtonPress,{390,15},Qt::LeftButton,Qt::LeftButton);labelEvent(QEvent::MouseMove,{440,15},Qt::NoButton,Qt::LeftButton);labelEvent(QEvent::MouseButtonRelease,{440,15},Qt::LeftButton,Qt::NoButton);
 if(labelDrags)return 3;
 labelEvent(QEvent::MouseButtonPress,{10,15},Qt::LeftButton,Qt::LeftButton);labelEvent(QEvent::MouseMove,{80,15},Qt::NoButton,Qt::LeftButton);labelEvent(QEvent::MouseButtonRelease,{80,15},Qt::LeftButton,Qt::NoButton);
 if(labelDrags!=1)return 4;
 QWidget nested; nested.setAttribute(Qt::WA_DontShowOnScreen);nested.resize(500,300);
 QWidget mixer(&nested);mixer.setObjectName("frameEditAudioMeter");mixer.setGeometry(100,80,300,180);
 QWidget row(&mixer);row.setObjectName("frameMixerMeterTitle");row.setGeometry(0,0,300,32);
 QLabel elided(QString(),&row);elided.setObjectName("lblMixerMeter");elided.setGeometry(0,0,200,32);
 nested.show();mixer.show();row.show();elided.show();app.processEvents();
 int nativeDrags=0;Unbound::NativePanelTitleDrags nativeTitles(&nested);
 nativeTitles.wire(&mixer,"frameEditAudioMeter","Mixer",[&](QPoint){++nativeDrags;});
 auto nativeEvent=[&](QEvent::Type type,QPoint pos,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,pos,elided.mapToGlobal(pos),button,buttons,Qt::NoModifier);QApplication::sendEvent(&elided,&e);};
 nativeEvent(QEvent::MouseButtonPress,{25,15},Qt::LeftButton,Qt::LeftButton);nativeEvent(QEvent::MouseMove,{90,15},Qt::NoButton,Qt::LeftButton);nativeEvent(QEvent::MouseButtonRelease,{90,15},Qt::LeftButton,Qt::NoButton);
 if(nativeDrags!=1)return 5;
 nativeEvent(QEvent::MouseButtonPress,{190,15},Qt::LeftButton,Qt::LeftButton);nativeEvent(QEvent::MouseMove,{250,15},Qt::NoButton,Qt::LeftButton);nativeEvent(QEvent::MouseButtonRelease,{250,15},Qt::LeftButton,Qt::NoButton);
 if(nativeDrags!=1)return 6;
 QList<Unbound::PanelHoverSurface> targets{{1,"Timeline",&nested},{8,"Mixer",&mixer}};
 if(Unbound::panelHoverTarget(&elided,targets)!=8 || Unbound::panelHoverTarget(&nested,targets)!=1 || Unbound::panelHoverTarget(nullptr,targets)!=-1)return 7;
 mixer.hide();if(Unbound::panelHoverTarget(&elided,targets)==8)return 8;
 std::puts("PASS: native title click retained; threshold drag dispatches once without an accidental click");return 0;
}
