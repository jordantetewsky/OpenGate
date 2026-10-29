// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractScrollArea>
#include <QApplication>
#include <QMetaMethod>
#include <QPointer>
#include <QScrollBar>
#include <QSet>
#include <QElapsedTimer>
#include <QScopedValueRollback>
#include <cstdlib>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include "diagnostics.h"
#include "../../core/nativetabviewport.h"
namespace Unbound {
// Experimental native adapter. Never reparent tabs or change their selection.
// Only a discovered native scroll model or Qt's original arrow buttons are used.
class TimelineTabScroller : public QObject {
public:
    explicit TimelineTabScroller(QWidget *tabs):QObject(tabs),tabs(tabs) {
        qtTabs=qobject_cast<QTabBar *>(tabs);
        if(qtTabs){
            for(auto button:tabs->findChildren<QToolButton *>(QString(),Qt::FindDirectChildrenOnly)){
                if(button->arrowType()==Qt::LeftArrow)left=button;
                if(button->arrowType()==Qt::RightArrow)right=button;
            }
        } else {
            auto areas=tabs->findChildren<QAbstractScrollArea *>();
            if(areas.size()==1)native=areas.first()->horizontalScrollBar();
        }
        if((!qtTabs || !left || !right)&&!native){inspect();return;}
        scroll=new QScrollBar(Qt::Horizontal,tabs);scroll->setObjectName("openGateTimelineTabScroll");
        scroll->setFocusPolicy(Qt::NoFocus);scroll->setToolTip("Scroll timeline tabs without changing the selected timeline");
        scroll->setAccessibleName("Timeline tabs horizontal scroll");scroll->setFixedHeight(7);
        refresh.setSingleShot(true);refresh.setInterval(0);
        connect(&refresh,&QTimer::timeout,this,[this]{sync();});
        connect(scroll,&QScrollBar::valueChanged,this,[this](int target){if(!writingWheel)wheelTracking=false;requested=target;if(!dispatch.isActive())dispatch.start();});
        connect(scroll,&QScrollBar::sliderReleased,this,[this]{sync();});
        dispatch.setSingleShot(true);dispatch.setInterval(0);connect(&dispatch,&QTimer::timeout,this,[this]{apply();});
        if(native){connect(native,&QScrollBar::rangeChanged,this,[this]{schedule();});connect(native,&QScrollBar::valueChanged,this,[this]{schedule();});}
        if(qtTabs){
            connect(qtTabs,&QTabBar::currentChanged,this,[this]{schedule();});
            connect(left,&QToolButton::clicked,this,[this]{schedule();});
            connect(right,&QToolButton::clicked,this,[this]{schedule();});
        }
        logEvent("timeline.tabs.capability",{{"supported",true},{"route",native?"native-scroll-model":"qt-native-arrows"},{"native_state_changed",false}});
        tabs->installEventFilter(this);schedule();
    }
    ~TimelineTabScroller()override{delete scroll.data();}
    bool supported()const{return scroll;}
    static bool isNativeTimelineTabs(QWidget *widget){
        if(!widget || QByteArray(widget->metaObject()->className())!="UiTiTimelineTabBar")return false;
        for(auto p=widget->parentWidget();p;p=p->parentWidget())if(p->objectName()=="timelineTabsHolder")return true;
        return false;
    }
protected:
    bool eventFilter(QObject *,QEvent *event)override{
        if(event->type()==QEvent::Wheel && scroll && scroll->maximum()>scroll->minimum()){
            auto wheel=static_cast<QWheelEvent *>(event);if(wheel->modifiers()!=Qt::NoModifier)return false;
            const auto pixels=wheel->pixelDelta();const auto angle=wheel->angleDelta();
            const int delta=!pixels.isNull()?(pixels.x()?pixels.x():pixels.y()):(angle.x()?angle.x():angle.y())/3;
            if(delta){
                if(!wheelTracking || !wheelClock.isValid() || wheelClock.elapsed()>300 || wheel->phase()==Qt::ScrollBegin)wheelTarget=offset();
                wheelTracking=true;wheelClock.restart();
                wheelTarget=qBound(scroll->minimum(),wheelTarget-delta,scroll->maximum());
                QScopedValueRollback<bool> guard(writingWheel,true);scroll->setValue(wheelTarget);
                if(wheel->phase()==Qt::ScrollEnd)wheelTracking=false;
                event->accept();return true;
            }
            if(wheelTracking && wheel->phase()==Qt::ScrollEnd){wheelTracking=false;event->accept();return true;}
        }
        if(event->type()==QEvent::Resize||event->type()==QEvent::LayoutRequest||event->type()==QEvent::Show||event->type()==QEvent::FontChange||event->type()==QEvent::StyleChange)schedule();
        return false;
    }
private:
    void schedule(){if(scroll&&!refresh.isActive())refresh.start();}
    int offset()const{return native?native->value():qtTabs&&qtTabs->count()?qMax(0,-qtTabs->tabRect(0).left()):0;}
    void sync(){
        if(!tabs||!scroll)return;
        int width=tabs->width();
        if(qtTabs)for(auto button:{left,right})if(button&&button->isVisible())width=qMin(width,button->geometry().left());
        const int span=qtTabs&&qtTabs->count()?qtTabs->tabRect(qtTabs->count()-1).right()-qtTabs->tabRect(0).left()+1:0;
        if(qtTabs && (lastWidth!=tabs->width() || lastCount!=qtTabs->count() || lastSpan!=span)) {
            stops.clear();stops.insert(0);lastWidth=tabs->width();lastCount=qtTabs->count();lastSpan=span;
        }
        if(qtTabs)stops.insert(offset());
        int maximum=0;
        if(native)maximum=native->maximum();
        else if(qtTabs&&qtTabs->count())maximum=qMax(0,qtTabs->tabRect(qtTabs->count()-1).right()+1+offset()-width);
        {QSignalBlocker block(scroll);scroll->setRange(native?native->minimum():0,maximum);scroll->setPageStep(qMax(1,width));if(!scroll->isSliderDown())scroll->setValue(offset());}
        scroll->setGeometry(0,qMax(0,tabs->height()-7),qMax(0,width),7);scroll->setVisible(maximum>scroll->minimum()&&width>=40);scroll->raise();
    }
    void apply(){
        if(!tabs||!scroll)return;
        if(native){native->setValue(requested);sync();return;}
        // Bounded native arrow steps; selected timeline and tab ordering are untouched.
        const int selected=qtTabs->currentIndex();int desired=requested;
        // Native arrows expose discrete stops. A pixel target between two stops
        // must not bounce left/right on each valueChanged while the user drags.
        if(!stops.isEmpty()) {
            int low=offset(),high=offset(),nearest=offset();
            for(int stop:stops){low=qMin(low,stop);high=qMax(high,stop);if(std::abs(stop-requested)<std::abs(nearest-requested))nearest=stop;}
            if(requested>=low&&requested<=high)desired=nearest;
        }
        for(int i=0;i<16;++i){
            const int before=offset();auto button=desired>before?right:left;
            if(desired==before||!button||!button->isEnabled())break;
            button->click();if(!qtTabs)return;const int after=offset();stops.insert(before);stops.insert(after);
            if(after==before)break;
            if(desired>=qMin(before,after)&&desired<=qMax(before,after)) {
                if(std::abs(before-desired)<std::abs(after-desired)) {
                    auto back=after>before?left:right;if(back&&back->isEnabled())back->click();
                    if(!qtTabs)return;stops.insert(offset());
                }
                break;
            }
        }
        logEvent("timeline.tabs.scroll",{{"requested",requested},{"offset",offset()},
            {"selection_unchanged",qtTabs->currentIndex()==selected}});sync();
    }
    void inspect(){
        QJsonArray methods,children;
        for(int i=tabs->metaObject()->methodOffset();i<tabs->metaObject()->methodCount()&&methods.size()<64;++i){
            auto signature=tabs->metaObject()->method(i).methodSignature();
            if(signature.toLower().contains("scroll")||signature.toLower().contains("tab"))methods.append(QString::fromLatin1(signature));
        }
        for(auto child:tabs->findChildren<QWidget *>()){
            if(children.size()>=64)break;
            QJsonObject state{{"class",child->metaObject()->className()},{"name",child->objectName()}};
            if(auto button=qobject_cast<QToolButton *>(child))state["arrow_type"]=int(button->arrowType());
            children.append(state);
        }
        logEvent("timeline.tabs.capability",{{"supported",false},{"class",tabs->metaObject()->className()},
            {"methods",methods},{"children",children},{"native_state_changed",false}});
    }
    QPointer<QWidget> tabs;QPointer<QTabBar> qtTabs;QPointer<QToolButton> left,right;
    QPointer<QScrollBar> native,scroll;QTimer refresh,dispatch;QSet<int> stops;QElapsedTimer wheelClock;
    int requested=0,lastWidth=-1,lastCount=-1,lastSpan=-1,wheelTarget=0;bool wheelTracking=false,writingWheel=false;
};
class TimelineTabScrolling : public QObject {
public:
    explicit TimelineTabScrolling(QObject *owner):QObject(owner){qApp->installEventFilter(this);scan.setSingleShot(true);scan.setInterval(100);connect(&scan,&QTimer::timeout,this,[this]{discover();});scan.start();}
    ~TimelineTabScrolling()override{if(qApp)qApp->removeEventFilter(this);for(auto adapter:adapters)if(adapter)delete adapter;}
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Show)if(auto widget=qobject_cast<QWidget *>(object))if(TimelineTabScroller::isNativeTimelineTabs(widget)&&!scan.isActive())scan.start();
        return false;
    }
private:
    void discover(){
        for(auto widget:QApplication::allWidgets())if(TimelineTabScroller::isNativeTimelineTabs(widget)&&!adapters.contains(widget)){
            if(qEnvironmentVariable("OPENGATE_TIMELINE_TAB_SCROLLING").isEmpty() || qEnvironmentVariable("OPENGATE_TIMELINE_TAB_SCROLLING")=="smooth") {
                auto continuous=new NativeTabViewport(this);
                if(continuous->attach(qobject_cast<QTabBar *>(widget))){adapters[widget]=continuous;logEvent("timeline.tabs.capability",{{"supported",true},{"route","experimental-public-qt-viewport"}});}
                else {delete continuous;logEvent("timeline.tabs.viewport.unavailable",{{"reason","unsupported-native-tab-layout"}});adapters[widget]=new TimelineTabScroller(widget);}
            } else adapters[widget]=new TimelineTabScroller(widget);
            connect(widget,&QObject::destroyed,this,[this,widget]{auto adapter=adapters.take(widget);if(adapter && adapter->parent()==this)adapter->deleteLater();});
        }
    }
    QHash<QWidget *,QPointer<QObject>> adapters;QTimer scan;
};
}
